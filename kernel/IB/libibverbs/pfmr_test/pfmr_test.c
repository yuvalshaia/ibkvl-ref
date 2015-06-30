#include <linux/types.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/list.h>         /* for list handling */
#include <linux/slab.h>         /* kmalloc */
#include <linux/vmalloc.h>      /* for vmalloc */

#include "pfmr_test.h"

static int fmr_idx = 1;
static int cq_idx = 1;
static int qp_idx = 1;
static int dma_idx = 1;

static int prep_qp_to_com(struct ib_qp *qp, int port, u16 dlid, u32 dqpn);
unsigned long str_to_ulong(const char *val, int base);
long str_to_long(const char *val, int base);
int rdma_send_fmre(struct ib_qp *qp, struct fmr_entry *fmre, u32 rkey,
		   u64 raddr);
int rdma_send(struct ib_qp *qp, u32 lkey, u64 va, int size, u32 rkey,
	      u64 raddr);

unsigned long str_to_ulong(const char *str_val, int base)
{
	if (!str_val)
		return -1;
	return simple_strtoull(str_val, 0, base);
}

long str_to_long(const char *str_val, int base)
{
	if (!str_val)
		return -1;
	return simple_strtol(str_val, 0, base);
}

int pfmr_create_fmr(void* kvldata, void *data, void* userbuff, unsigned int len)
{
	struct kvl_op *op = (struct kvl_op *)kvldata;
	struct pfmr_test_ctx *test_ctx = data;
	int rc, i;
	int nfmr = get_param_intval(op, "nfmr");
	int max_pages = get_param_intval(op, "max_pages");
	int max_maps = get_param_intval(op, "max_maps");
	int show = get_param_intval(op, "show");
	rc = 0;
	if (show) {
		struct list_head *cursor, *tmp;
		i = 0;
		info("[%s] printing FMR list:\n",test_ctx->ibdev->name);
		list_for_each_safe(cursor, tmp, &test_ctx->fmr_list) {
			struct fmr_entry *fmre = (struct fmr_entry*)cursor;
			info("\t FMR[%d] : lkey=0x%x rkey=0x%x\n",
			     fmre->index, be32_to_cpu(fmre->fmr->lkey),
			     be32_to_cpu(fmre->fmr->rkey));
		}
		return 0;
	}
	for (i = 0; i < nfmr; i++) {
		int fmr_flags = IB_ACCESS_LOCAL_WRITE |
				IB_ACCESS_REMOTE_WRITE |
				IB_ACCESS_REMOTE_READ |
				IB_ACCESS_REMOTE_ATOMIC;

		struct ib_fmr_attr fmr_props = {
			.max_maps = max_maps,
			.max_pages = max_pages,
			.page_shift = PAGE_SHIFT,
		};

		struct fmr_entry *fmre = kzalloc(sizeof(struct fmr_entry), 0);
		if (!fmre) {
			error("[%s] Failed to allocate fmr entry\n",
			      test_ctx->ibdev->name);
			return -ENOMEM;
		}
		fmre->fmr = ib_alloc_fmr(test_ctx->pd, fmr_flags, &fmr_props);
		if (!fmre->fmr || IS_ERR(fmre->fmr)) {
			error("[%s] Failed to allocate fmr %d\n",
			      test_ctx->ibdev->name, i);
			kfree(fmre);
			return -1;
		}
		fmre->index = fmr_idx++;
		debug("[%s] FMR[%d] %p created successfully\n",
		      test_ctx->ibdev->name, fmre->index, fmre->fmr);
		list_add_tail(&fmre->list, &test_ctx->fmr_list);
	}
	info("protected FMR test on IB dev=%s nfmr=%d ...\n",
	     test_ctx->ibdev->name, nfmr);
	return rc;
}

int pfmr_create_dma_buff(void* kvldata, void *data, void* userbuff, unsigned int len)
{
	struct kvl_op* op = (struct kvl_op*) kvldata;
	struct pfmr_test_ctx *test_ctx = data;
	int rc,i;
	int ndma = get_param_intval(op, "ndma");
	int show = get_param_intval(op,"show");
	rc = 0;
	if (show) {
		struct list_head *cursor, *tmp;
		info("[%s] printing DMA buff list:\n", test_ctx->ibdev->name);
		list_for_each_safe(cursor, tmp, &test_ctx->dma_list) {
			struct dma_entry *dmae = (struct dma_entry*)cursor;
			info("\t dma[%d]: size=%d va=0x%llx pa=0x%llx\n",
			     dmae->index, dmae->size , (u64)dmae->va,
			     (u64)dmae->pa);
		}
		return 0;
	}
	for (i = 0; i < ndma; i++) {
		struct dma_entry *dmae = kzalloc(sizeof(struct dma_entry),
						 GFP_KERNEL);
		if (!dmae) {
			error("[%s] Failed to allocate dma entry\n",
			      test_ctx->ibdev->name);
			return -ENOMEM;
		}
		dmae->index = dma_idx++;
		dmae->size = PAGE_SIZE;
		debug("alloc MDA buff\n");
		dmae->buff = kzalloc(dmae->size, GFP_DMA);
		if (!dmae->buff) {
			error("[%s] Failed to allocate DMA buffer %d\n",
			      test_ctx->ibdev->name, dmae->index);
			kfree(dmae);
			dma_idx--;
			return -1;
		}
		memset(dmae->buff, 0, dmae->size);
		dmae->va = (void *)PAGE_ALIGN((u64)dmae->buff);
		debug("alloc DMA MAP %llx (was %llx), size=%d\n", (u64)dmae->va,
		      (u64)dmae->buff, dmae->size);
		dmae->pa = ib_dma_map_single(test_ctx->ibdev, dmae->va,
					     dmae->size, DMA_BIDIRECTIONAL);
		rc = dma_mapping_error(test_ctx->ibdev->dma_device, dmae->pa);
		if (rc) {
			error("[%s] failed to DMA map buffer %d rc=%d\n",
			      test_ctx->ibdev->name, dmae->index, rc);
			dmae->pa = 0;
			kfree(dmae->buff);
			kfree(dmae);
			dma_idx--;
			return rc;
		}
		debug("[%s] DMA[%d] %llx size=%d created & DMA mapped "
		      "successfully\n", test_ctx->ibdev->name, dmae->index,
		      (u64)dmae->va, dmae->size);
		list_add_tail(&dmae->list, &test_ctx->dma_list);
		info("[%s] Created DMA buff[%d] va=0x%lx pa=0x%llx\n",
		     test_ctx->ibdev->name, dmae->index,
		     (unsigned long)dmae->va, dmae->pa);
	}
	return rc;
}

static struct fmr_entry *find_fmr_entry(struct pfmr_test_ctx *test_ctx, int index)
{
	struct list_head *cursor, *tmp;
	list_for_each_safe(cursor, tmp, &test_ctx->fmr_list) {
		struct fmr_entry *fmre = (struct fmr_entry*)cursor;
		if(fmre->index == index)
			return fmre;
	}
	error("[%s] No FMR with index %d is found !\n",
		test_ctx->ibdev->name, index);
	return NULL;
}

static struct dma_entry *find_dma_entry(struct pfmr_test_ctx *test_ctx, int index)
{
	struct list_head *cursor, *tmp;
	list_for_each_safe(cursor, tmp, &test_ctx->dma_list) {
		struct dma_entry *dmae = (struct dma_entry*)cursor;
		if(dmae->index == index)
			return dmae;
	}
	error("[%s] No DMA with index %d is found !\n",
		test_ctx->ibdev->name, index);
	return NULL;
}

static struct cq_entry *find_cq_entry(struct pfmr_test_ctx *test_ctx, int index)
{
	struct list_head *cursor, *tmp;
	list_for_each_safe(cursor, tmp, &test_ctx->cq_list) {
		struct cq_entry *cqe = (struct cq_entry *)cursor;
		if (cqe->index == index)
			return cqe;
	}
	error("[%s] No CQ with index %d is found !\n",
		test_ctx->ibdev->name, index);
	return NULL;
}

static struct qp_entry *find_qp_entry(struct pfmr_test_ctx *test_ctx, int index)
{
	struct list_head *cursor, *tmp;
	list_for_each_safe(cursor, tmp, &test_ctx->qp_list) {
		struct qp_entry *qpe = (struct qp_entry *)cursor;
		if (qpe->index == index)
			return qpe;
	}
	error("[%s] No QP with index %d is found !\n",
		test_ctx->ibdev->name, index);
	return NULL;
}

int pfmr_dma_ops(void *kvldata, void *data, void *userbuff, unsigned int len)
{
	struct kvl_op *op = (struct kvl_op *)kvldata;
	struct pfmr_test_ctx *test_ctx = data;
	int dma_idx;
	struct dma_entry *dmae;
	const char *new_val;

	dma_idx = get_param_intval(op, "dma_idx");

	dmae = find_dma_entry(test_ctx, dma_idx);
	if (!dmae)
		return -ENODATA;

	/* Destroy DMA */
	if (get_param_intval(op, "des")) {
		info("[%s] Destroying DMA buff[%d] va=0x%lx pa=0x%llx\n",
		     test_ctx->ibdev->name, dmae->index,
		     (unsigned long)dmae->va, dmae->pa);
		ib_dma_unmap_single(test_ctx->ibdev, dmae->pa, dmae->size,
				    DMA_BIDIRECTIONAL);
		kfree(dmae->buff);
		list_del(&dmae->list);
		return 0;
	}

	/* Read DMA value */
	if (get_param_intval(op, "get")) {
		char *buff = kmalloc(dmae->size, GFP_KERNEL);
		ib_dma_sync_single_for_cpu(test_ctx->ibdev, dmae->pa,
					   dmae->size, DMA_BIDIRECTIONAL);
		memcpy(buff, dmae->va, dmae->size);
		info("[%s] Buffer: %s\n", test_ctx->ibdev->name, buff);
		ib_dma_sync_single_for_device(test_ctx->ibdev, dmae->pa,
					      dmae->size, DMA_BIDIRECTIONAL);
		kfree(buff);
		return 0;
	}

	/* Set DMA value */
	new_val = get_param_strval(op, "set");
	if (*new_val != '~') {
		int i;
		ib_dma_sync_single_for_cpu(test_ctx->ibdev, dmae->pa,
					   dmae->size, DMA_BIDIRECTIONAL);
		for (i = 0; (i < dmae->size) && (*new_val); i++, new_val++)
			*(char *)(dmae->va + i) = *new_val;
		*(char *)(dmae->va + i) = 0;
		ib_dma_sync_single_for_device(test_ctx->ibdev, dmae->pa,
					      dmae->size, DMA_BIDIRECTIONAL);
		info("[%s] Buffer modifyed\n", test_ctx->ibdev->name);
	}

	return 0;
}

int pfmr_destroy_fmr(void *kvldata, void *data, void *userbuff,
		     unsigned int len)
{
	struct kvl_op *op = (struct kvl_op *)kvldata;
	struct pfmr_test_ctx *test_ctx = data;
	int fmr_idx = get_param_intval(op, "fmr_idx");
	struct fmr_entry *fmre;
	int rc = 0;

	fmre = find_fmr_entry(test_ctx, fmr_idx);
	if (!fmre)
		return -ENODATA;

	debug("[%s] FMR[%d] %p destroyed successfully\n",
	      test_ctx->ibdev->name, fmre->index, fmre->fmr);
	rc = ib_dealloc_fmr(fmre->fmr);
	if (rc) {
		error("FATAL : Failed to dealloc FMR lkey=%x rc=%d\n",
		      fmre->fmr->lkey, rc);
		return -ENODATA;
	}
	list_del(&fmre->list);

	return 0;
}

int pfmr_map_dma_fmr(void* kvldata, void *data, void* userbuff, unsigned int len)
{
	struct kvl_op* op = (struct kvl_op*) kvldata;
	struct pfmr_test_ctx *test_ctx = data;
	int rc = 0;
	int fmr_idx = get_param_intval(op, "fmr_idx");
	int dma_idx = get_param_intval(op,"dma_idx");
	int show = get_param_intval(op,"show");
	struct fmr_entry *fmre;
	struct dma_entry *dmae;
	u64 *page_list;

	if (show) {
		struct list_head *cursor, *tmp;
		info("[%s] printing FMR DMA Mapping list:\n",
		     test_ctx->ibdev->name);
		list_for_each_safe(cursor, tmp, &test_ctx->fmr_list) {
			struct fmr_entry *fmre = (struct fmr_entry*)cursor;
			struct dma_entry *dmae = fmre->dma_map;
			if (!dmae) 
				continue;
			info("\t FMR[%d] lkey=0x%x rkey=0x%x (%d) DMA[%d]-> "
			     "\n\t\t\tsize=%d va=0x%llx pa=0x%llx\n",
			     fmre->index, be32_to_cpu(fmre->fmr->lkey),
			     be32_to_cpu(fmre->fmr->rkey),
			     be32_to_cpu(fmre->fmr->rkey),
			     dmae->index , dmae->size, (u64)dmae->va, dmae->pa);
		}
		return 0;
	}
	fmre = find_fmr_entry(test_ctx, fmr_idx);
	if (!fmre)
		return -ENODATA;

	dmae = find_dma_entry(test_ctx, dma_idx);
	if (!dmae)
		return -ENODATA;

	if (get_param_intval(op, "unmap")) {
		if (!fmre->dma_map) {
			error("[%s] not mapped\n", test_ctx->ibdev->name);
			return -ENODATA;
		}
		LIST_HEAD(fmr_list);
		list_add(&fmre->fmr->list, &fmr_list);
		rc = ib_unmap_fmr(&fmr_list);
		if (rc) {
			error("[%s] Failed to ib_unmap_fmr FMR %x to DMA[%d] "
			      "size=%d va=0x%llx pa=0x%llx\n",
			      test_ctx->ibdev->name,
			      cpu_to_be32(fmre->fmr->lkey), dmae->index,
			      dmae->size , (u64)dmae->va, dmae->pa);
			return rc;
		}
		fmre->dma_map = 0;
		/* TODO: delete from list */
		return 0;
	}

	debug("MAPPING PHYS FMR\n");
	fmre->dma_map = 0;
	page_list = vmalloc(sizeof(u64));
	page_list[0] = dmae->pa;
	rc = ib_map_phys_fmr(fmre->fmr, page_list, 1, (u64)dmae->va);
	vfree(page_list);
	debug("PHYS FMR MAPPED rc=%d\n", rc);
	if (rc) {
		error("[%s] Failed to ib_map_phys_fmr FMR %x to DMA[%d] size=%d"
		      " va=0x%llx pa=0x%llx\n", test_ctx->ibdev->name,
		      cpu_to_be32(fmre->fmr->lkey), dmae->index, dmae->size,
		      (u64)dmae->va, dmae->pa);
		return rc;
	}
	fmre->dma_map = dmae;
	info("[%s] MAPPING FMR %x to DMA[%d] size=%d va=0x%llx pa=0x%llx\n",
	     test_ctx->ibdev->name, cpu_to_be32(fmre->fmr->lkey), dmae->index,
	     dmae->size, (u64)dmae->va, dmae->pa);
	return rc;
}


struct pfmr_test_ctx *pfmr_create_test_ctx(struct ib_device *ibdev)
{
	struct pfmr_test_ctx *test_ctx;
	test_ctx = kzalloc(sizeof(struct pfmr_test_ctx), GFP_KERNEL);
	if (!test_ctx) {
		error("[%s] Failed to allocate test context\n", ibdev->name);
		return NULL;
	}
	INIT_LIST_HEAD(&test_ctx->fmr_list);
	INIT_LIST_HEAD(&test_ctx->dma_list);
	INIT_LIST_HEAD(&test_ctx->cq_list);
	INIT_LIST_HEAD(&test_ctx->qp_list);
	test_ctx->ibdev = ibdev;
	test_ctx->pd = ib_alloc_pd(ibdev);
	if (!test_ctx->pd) {
		error("[%s] Failed to allocate PD\n", ibdev->name);
		goto err;
	}
	debug("[%s] PD created successfully\n", ibdev->name);
	return test_ctx;
err:
	pfmr_destroy_test_ctx(test_ctx);
	return NULL;
}

void pfmr_destroy_test_ctx(struct pfmr_test_ctx *test_ctx)
{
	if (test_ctx) {
		int rc;
		struct list_head *cursor, *tmp;
		/* FMRs & FMR mappings */
		list_for_each_safe(cursor, tmp, &test_ctx->fmr_list){
			struct fmr_entry *fmre = (struct fmr_entry*)cursor;
			list_del(cursor);
			if (fmre->fmr && !IS_ERR(fmre->fmr)){
				if (fmre->dma_map) {
					LIST_HEAD(fmr_list);
					list_add(&fmre->fmr->list,&fmr_list);
					ib_unmap_fmr(&fmr_list);
				}
				rc = ib_dealloc_fmr(fmre->fmr);
				if (rc) {
					error("FATAL : Failed to dealloc FMR "
					      "lkey=%x rc=%d\n",
					      fmre->fmr->lkey, rc);
				}
			}
			kfree(fmre);
		}
		/* DMAs */
		list_for_each_safe(cursor, tmp, &test_ctx->dma_list){
			struct dma_entry *dmae = (struct dma_entry*)cursor;
			list_del(cursor);
			if (dmae->pa)
				dma_unmap_single(test_ctx->ibdev->dma_device,
						 dmae->pa, dmae->size,
						 DMA_BIDIRECTIONAL);
			kfree(dmae->buff);
			kfree(dmae);
		}
		/* QPs */
		list_for_each_safe(cursor, tmp, &test_ctx->qp_list) {
			struct qp_entry *qpe = (struct qp_entry *)cursor;
			ib_destroy_qp(qpe->qp);
			list_del(cursor);
			kfree(qpe);
		}
		/* CQs */
		list_for_each_safe(cursor, tmp, &test_ctx->cq_list) {
			struct cq_entry *cqe = (struct cq_entry *)cursor;
			ib_destroy_cq(cqe->cq);
			list_del(cursor);
			kfree(cqe);
		}
		/* PD */
		if (test_ctx->pd) 
			ib_dealloc_pd(test_ctx->pd);
		test_ctx->pd = NULL;

		kfree(test_ctx);
	}
	debug("[%s] pfmr test context was destroyed\n", test_ctx->ibdev->name);
}

int pfmr_create_pool(void* kvldata, void *data, void* userbuff, unsigned int len)
{
	struct kvl_op* op = (struct kvl_op*) kvldata;
	struct pfmr_test_ctx *test_ctx = data;
	int size = get_param_intval(op, "size");
	struct ib_fmr_pool *fmr_pool;
	struct ib_fmr_pool_param fmr_param;
	memset(&fmr_param, 0, sizeof fmr_param);
	fmr_param.pool_size = size;
	fmr_param.cache = 1;
	fmr_param.page_shift = PAGE_SHIFT;
	fmr_param.dirty_watermark = (0.85*1024.0);
	fmr_param.max_pages_per_fmr = 256;
	fmr_param.flush_function = NULL;
	fmr_param.access = IB_ACCESS_LOCAL_WRITE |
				       IB_ACCESS_REMOTE_WRITE |
				       IB_ACCESS_REMOTE_READ;

	info("Creating fmr pool size=%d\n",fmr_param.pool_size);
	fmr_pool = ib_create_fmr_pool(test_ctx->pd, &fmr_param);
	if (!fmr_pool|| IS_ERR(fmr_pool)) {
		error("Failed to create FMR pool rc=%ld\n", PTR_ERR(fmr_pool));
		return PTR_ERR(fmr_pool);
	}

	info("Destroy fmr pool %p\n", fmr_pool);
	ib_destroy_fmr_pool(fmr_pool);
	return  0;
}

int pfmr_cq_op(void *kvldata, void *data, void *userbuff, unsigned int len)
{
	struct kvl_op *op = (struct kvl_op *) kvldata;
	struct pfmr_test_ctx *test_ctx = data;
	int idx, i;
	int cq_count;
	struct cq_entry *cqe;
	int rc = 0;

	/* Show */
	if (get_param_intval(op, "show")) {
		struct list_head *cursor, *tmp;
		info("[%s] printing CQ list:\n", test_ctx->ibdev->name);
		list_for_each_safe(cursor, tmp, &test_ctx->cq_list) {
			cqe = (struct cq_entry *)cursor;
			info("\t CQ[%d] : %p\n", cqe->index, cqe->cq);
		}
		return 0;
	}

	/* Create */
	cq_count = get_param_intval(op, "cr");
	i = 0;
	if (cq_count) {
		for (i = 0; i < cq_count; i++) {
			cqe = kzalloc(sizeof(struct cq_entry), 0);
			if (!cqe) {
				error("[%s] Failed to allocate CQ entry\n",
				      test_ctx->ibdev->name);
				return -ENOMEM;
			}
			cqe->cq = ib_create_cq(test_ctx->ibdev, NULL, NULL,
					       NULL, 4, 0);
			if (!cqe->cq || IS_ERR(cqe->cq)) {
				error("[%s] Failed to allocate CQ %d\n",
				      test_ctx->ibdev->name, i);
				kfree(cqe);
				return -1;
			}
			cqe->index = cq_idx++;
			debug("[%s] CQ[%d] %p created successfully\n",
			      test_ctx->ibdev->name, cqe->index, cqe->cq);
			list_add_tail(&cqe->list, &test_ctx->cq_list);
		}
		return 0;
	}

	/* Destroy */
	idx = get_param_intval(op, "des");
	if (idx) {
		cqe = find_cq_entry(test_ctx, idx);
		if (!cqe)
			return -ENODATA;

		rc = ib_destroy_cq(cqe->cq);
		if (rc) {
			error("FATAL : Failed to destroy CQ rc=%d\n", rc);
			return -ENODATA;
		}
		list_del(&cqe->list);
		debug("[%s] CQ[%d] %p Removed\n",
		      test_ctx->ibdev->name, cqe->index, cqe->cq);
		kfree(cqe);
		return 0;
	}

	return 0;
}

static int prep_qp_to_com(struct ib_qp *qp, int port, u16 dlid, u32 dqpn)
{
	int qp_attr_mask = 0;
	struct ib_qp_attr qp_attr;
	int rc = -1;
	static u32 psn = 1;

	memset(&qp_attr, 0, sizeof(struct ib_qp_attr));

	/* RESET */
	qp_attr_mask |= IB_QP_STATE;
	qp_attr.qp_state = IB_QPS_RESET;
	rc = ib_modify_qp(qp, &qp_attr, qp_attr_mask);
	if (rc)
		goto err;

	/* INIT */
	memset(&qp_attr, 0, sizeof(struct ib_qp_attr));
	qp_attr_mask = 0;
	qp_attr_mask |= IB_QP_STATE;
	qp_attr.qp_state = IB_QPS_INIT;
	qp_attr_mask |= IB_QP_PKEY_INDEX;
	qp_attr.pkey_index = 0;
	qp_attr_mask |= IB_QP_PORT;
	qp_attr.port_num = port;
	qp_attr_mask |= IB_QP_ACCESS_FLAGS;
	qp_attr.qp_access_flags = IB_ACCESS_REMOTE_WRITE |
				  IB_ACCESS_REMOTE_READ;
	rc = ib_modify_qp(qp, &qp_attr, qp_attr_mask);
	if (rc)
		goto err;

	/* RTR */
	qp_attr_mask = 0;
	qp_attr_mask |= IB_QP_STATE;
	qp_attr.qp_state = IB_QPS_RTR;
	qp_attr_mask |= IB_QP_AV;
	qp_attr.ah_attr.sl = 0;
	qp_attr.ah_attr.ah_flags = 0;
	qp_attr.ah_attr.dlid = dlid;
	qp_attr.ah_attr.static_rate = 2;
	qp_attr.ah_attr.src_path_bits = 0;
	qp_attr_mask |= IB_QP_PATH_MTU;
	qp_attr.path_mtu = IB_MTU_256;
	qp_attr_mask |= IB_QP_RQ_PSN;
	qp_attr.rq_psn = psn;
	qp_attr_mask |= IB_QP_MAX_DEST_RD_ATOMIC;
	qp_attr.max_dest_rd_atomic = 1;
	qp_attr_mask |= IB_QP_DEST_QPN;
	qp_attr.dest_qp_num = dqpn;
	qp_attr_mask |= IB_QP_MIN_RNR_TIMER;
	qp_attr.min_rnr_timer = 0;
	rc = ib_modify_qp(qp, &qp_attr, qp_attr_mask);
	if (rc)
		goto err;

	/* RTS */
	qp_attr_mask = 0;
	qp_attr_mask |= IB_QP_STATE;
	qp_attr.qp_state = IB_QPS_RTS;
	qp_attr_mask |= IB_QP_SQ_PSN;
	qp_attr.sq_psn = psn;
	qp_attr_mask |= IB_QP_TIMEOUT;
	qp_attr.timeout = 0x4;
	qp_attr_mask |= IB_QP_RETRY_CNT;
	qp_attr.retry_cnt = 0;
	qp_attr_mask |= IB_QP_RNR_RETRY;
	qp_attr.rnr_retry = 0;
	qp_attr_mask |= IB_QP_MAX_QP_RD_ATOMIC;
	qp_attr.max_rd_atomic = 1;
	rc = ib_modify_qp(qp, &qp_attr, qp_attr_mask);
	if (rc)
		goto err;

	/* increase the PSN every time we do modify QP */
	psn += 100000;

	return 0;
err:
	if (rc)
		error("Fail to switch to QP state %x, rc=%d\n",
		      qp_attr.qp_state, rc);
	return rc;
}

int pfmr_qp_op(void *kvldata, void *data, void* userbuff, unsigned int len)
{
	struct kvl_op *op = (struct kvl_op *)kvldata;
	struct pfmr_test_ctx *test_ctx = data;
	int idx, port;
	struct qp_entry *qpe;
	int rc = 0;

	/* Show */
	if (get_param_intval(op, "show")) {
		struct list_head *cursor, *tmp;
		info("[%s] printing QP list:\n", test_ctx->ibdev->name);
		list_for_each_safe(cursor, tmp, &test_ctx->qp_list) {
			qpe = (struct qp_entry *)cursor;
			info("\t QP[%d] : %p, qpn=0x%x, send_cq_idx=%d, "
			     "recv_cq_idx=%d\n", qpe->index, qpe->qp,
			     qpe->qp->qp_num, qpe->send_cq_index,
			     qpe->recv_cq_index);
		}
	}

	/* Create */
	if (get_param_intval(op, "cr")) {
		int send_cq_idx, recv_cq_idx;
		struct cq_entry *send_cqe, *recv_cqe;
		struct ib_qp_init_attr qp_init_attr;

		memset(&qp_init_attr, 0, sizeof(struct ib_qp_init_attr));

		send_cq_idx = get_param_intval(op, "scq_idx");
		recv_cq_idx = get_param_intval(op, "rcq_idx");
		send_cqe = find_cq_entry(test_ctx, send_cq_idx);
		recv_cqe = find_cq_entry(test_ctx, recv_cq_idx);
		if (!send_cqe || !recv_cqe)
			return -ENODATA;

		debug("Send CQ[%d] : %p\n", send_cqe->index, send_cqe->cq);
		debug("Recv CQ[%d] : %p\n", recv_cqe->index, recv_cqe->cq);
		qpe = kzalloc(sizeof(struct qp_entry), 0);
		if (!qpe) {
			error("[%s] Failed to allocate QP entry\n",
			      test_ctx->ibdev->name);
			return -ENOMEM;
		}
		qp_init_attr.send_cq = send_cqe->cq;
		qp_init_attr.recv_cq = recv_cqe->cq;
		qp_init_attr.cap.max_send_wr = 4;
		qp_init_attr.cap.max_recv_wr = 4;
		qp_init_attr.cap.max_send_sge = 1;
		qp_init_attr.cap.max_recv_sge = 1;
		qp_init_attr.sq_sig_type = IB_SIGNAL_ALL_WR;
		qp_init_attr.qp_type = IB_QPT_RC;
		qpe->qp = ib_create_qp(test_ctx->pd, &qp_init_attr);
		if (!qpe->qp || IS_ERR(qpe->qp)) {
			error("[%s] Failed to create PQ, (rc=%ld)\n",
			      test_ctx->ibdev->name,
			      qpe->qp ? PTR_ERR(qpe->qp) : 0);
			kfree(qpe);
			return -ENOMEM;
		}

		qpe->send_cq_index = send_cq_idx;
		qpe->recv_cq_index = recv_cq_idx;
		qpe->index = qp_idx++;
		debug("[%s] QP[%d] %p created successfully, qp_num=0x%x\n",
		      test_ctx->ibdev->name, qpe->index, qpe->qp,
		      qpe->qp->qp_num);
		list_add_tail(&qpe->list, &test_ctx->qp_list);
	}

	/* Prepare QP to send */
	idx = get_param_intval(op, "rts");
	if (idx) {
		int dqpn_idx;
		struct ib_port_attr port_attr;
		struct qp_entry *dqpe;
		u16 dlid;
		u32 dqpn;

		port = get_param_intval(op, "port");
		dqpn_idx = get_param_intval(op, "dqpn_idx");
		dlid = get_param_intval(op, "dlid");
		dqpn = str_to_long(get_param_strval(op, "dqpn"), 16);

		qpe = find_qp_entry(test_ctx, idx);
		if (!qpe)
			return -ENODATA;
		if (dqpn_idx) {
			dqpe = find_qp_entry(test_ctx, dqpn_idx);
			if (!dqpe)
				return -ENODATA;
			dqpn = dqpe->qp->qp_num;
		}

		if (!dlid) {
			rc = ib_query_port(test_ctx->ibdev, port, &port_attr);
			if (rc) {
				error("[%s] Failed to query port %d, (rc=%d)\n",
				      test_ctx->ibdev->name, port, rc);
			}
			dlid = port_attr.lid;
		}
		debug("[%s] dlid=%d\n", test_ctx->ibdev->name, dlid);

		rc = prep_qp_to_com(qpe->qp, port, dlid, dqpn);
		if (rc)
			return -ENODATA;
		debug("[%s] QP[%d] %p ready to send\n", test_ctx->ibdev->name,
		      qpe->index, qpe->qp);
	}

	/* RDMA */
	idx = get_param_intval(op, "rdma");
	if (idx) {
		int fmr_idx, rfmr_idx;
		u32 rkey;
		u64 raddr;			
		struct fmr_entry *fmre, *rfmre;

		qpe = find_qp_entry(test_ctx, idx);
		if (!qpe)
			return -ENODATA;

		fmr_idx = get_param_intval(op, "fmr_idx");
		fmre = find_fmr_entry(test_ctx, fmr_idx);
		if (!fmre)
			return -ENODATA;

		rfmr_idx = get_param_intval(op, "rfmr_idx");
		if (rfmr_idx) {
			rfmre = find_fmr_entry(test_ctx, rfmr_idx);
			if (!rfmre)
				return -ENODATA;
			if (!fmre->dma_map) {
				error("[%s] QP[%d] FMR %d is not mapped\n",
				      test_ctx->ibdev->name, qpe->index, fmre->index);
				return -EINVAL;
			}
			rkey = rfmre->fmr->rkey;
			raddr = (u64) rfmre->dma_map->va;
		} else {
			rkey = cpu_to_be32((u32)str_to_long(get_param_strval(op, "rkey"), 16));
			//rkey = cpu_to_be32(get_param_intval(op, "rkey"));
			raddr = str_to_ulong(get_param_strval(op, "raddr"), 16);
		}

		rc = rdma_send_fmre(qpe->qp, fmre, rkey, raddr);
		if (rc) {
			error("[%s] QP[%d] %p Fail to send, rc=%d\n",
			      test_ctx->ibdev->name, qpe->index, qpe->qp, rc);
			return -ENODATA;
		}
		debug("[%s] QP[%d] %p Sent done\n",
		      test_ctx->ibdev->name, qpe->index, qpe->qp);
	}

	/* Destroy */
	idx = get_param_intval(op, "des");
	if (idx) {
		qpe = find_qp_entry(test_ctx, idx);
		if (!qpe)
			return -ENODATA;

		debug("[%s] QP[%d] destroying %p\n",
		      test_ctx->ibdev->name, qpe->index, qpe->qp);
		rc = ib_destroy_qp(qpe->qp);
		if (rc) {
			error("FATAL : Failed to destroy QP rc=%d\n", rc);
			return -ENODATA;
		}
		list_del(&qpe->list);
		debug("[%s] QP[%d] %p Removed\n", test_ctx->ibdev->name,
		      qpe->index, qpe->qp);
		kfree(qpe);
	}

	return 0;
}

int rdma_send_fmre(struct ib_qp *qp, struct fmr_entry *fmre, u32 rkey,
		   u64 raddr)
{
	return rdma_send(qp, fmre->fmr->lkey, (u64) fmre->dma_map->va,
			 fmre->dma_map->size, rkey, raddr);
}

#define COMP_TIMEOUT 2000
int rdma_send(struct ib_qp *qp, u32 lkey, u64 va, int size, u32 rkey,
	      u64 raddr)
{
	struct ib_sge sge;
	struct ib_send_wr swr;
	struct ib_send_wr *bad_send_wr;
	unsigned long poll_total_time, poll_start_time;
	struct ib_wc comp_wc;
	int rc;

	memset(&sge, 0, sizeof(struct ib_sge));
	memset(&swr, 0, sizeof(struct ib_send_wr));

	sge.addr   		= va;
	sge.length 		= size;
	sge.lkey   		= lkey;

	swr.next		= NULL;
	swr.wr_id		= 1;
	swr.opcode		= IB_WR_RDMA_WRITE;
	swr.send_flags		= IB_SEND_SIGNALED;
	swr.sg_list		= &sge;
	swr.num_sge		= 1;
	swr.wr.rdma.remote_addr = raddr;
	swr.wr.rdma.rkey	= rkey;

	debug("rdma_send(lkey=0x%x, addr=0x%lx, length=%d)\n",
	      cpu_to_be32(sge.lkey), (long unsigned int)sge.addr, sge.length);
	debug("rdma_send(rkey=0x%x, raddr=0x%lx)\n", cpu_to_be32(rkey),
	      (long unsigned int)raddr);

	rc = ib_post_send(qp, &swr, &bad_send_wr);
	if (rc)
		return rc;

	memset(&comp_wc, 0, sizeof(struct ib_wc));
	poll_start_time = jiffies;
	do {
		poll_total_time = jiffies - poll_start_time;
		rc = ib_poll_cq(qp->send_cq, 1, &comp_wc);
		if (rc == 0) {
			if (jiffies_to_usecs(poll_total_time) <
			    (COMP_TIMEOUT * 1000))
				yield();
			else
				break;
		}
	} while (rc == 0);

	debug("rc=%d, wc.status=%d, vendor_err=0x%x\n", rc, comp_wc.status,
	      comp_wc.vendor_err);

	return !((rc == 1) && (comp_wc.status == IB_WC_SUCCESS));
}

int pfmr_complete_test(void* kvldata, void *data, void* userbuff, unsigned int len)
{
	struct pfmr_test_ctx *test_ctx = data;
	struct ib_fmr *fmr1, *fmr2;
	char *buf1, *buf2;
	int size;
	u64 dma1, dma2;
	struct ib_cq *cq;
	struct ib_qp *qp;
	struct ib_qp_init_attr qp_init_attr;
	int rc;
	int port;
	struct ib_port_attr port_attr;
	u64 *page_list;

	/* FMR */
	int fmr_flags = IB_ACCESS_LOCAL_WRITE |
			IB_ACCESS_REMOTE_WRITE |
			IB_ACCESS_REMOTE_READ;
	struct ib_fmr_attr fmr_props = {
		.max_maps = 10,
		.max_pages = 10,
		.page_shift = PAGE_SHIFT
	};
	fmr1 = ib_alloc_fmr(test_ctx->pd, fmr_flags, &fmr_props);
	if (!fmr1 || IS_ERR(fmr1)) {
		error("ib_alloc_fmr\n");
		return -1;
	}
	fmr2 = ib_alloc_fmr(test_ctx->pd, fmr_flags, &fmr_props);
	if (!fmr2 || IS_ERR(fmr2)) {
		error("ib_alloc_fmr\n");
		return -1;
	}

	/* VA */
	size = PAGE_SIZE * 3;
	buf1 = (void*)PAGE_ALIGN((u64)kzalloc(size, GFP_KERNEL));	
	memcpy(buf1, "buf1", 4);
	buf1[4] = 0;
	buf2 = (void*)PAGE_ALIGN((u64)kzalloc(size, GFP_KERNEL));	
	memcpy(buf2, "buf2", 4);
	buf2[4] = 0;
	info("[%s] buf2=%s\n", test_ctx->ibdev->name, buf2);

	/* DMA */
	dma1 = ib_dma_map_single(test_ctx->ibdev, buf1, size,
				 DMA_BIDIRECTIONAL);
	if (dma_mapping_error(test_ctx->ibdev->dma_device, dma1)) {
		error("ib_dma_map_single\n");
		return -1;
	}
	ib_dma_sync_single_for_device(test_ctx->ibdev, dma1, size,
				       DMA_BIDIRECTIONAL);
	dma2 = ib_dma_map_single(test_ctx->ibdev, buf2, size,
				 DMA_BIDIRECTIONAL);
	if (dma_mapping_error(test_ctx->ibdev->dma_device, dma2)) {
		error("ib_dma_map_single\n");
		return -1;
	}
	ib_dma_sync_single_for_device(test_ctx->ibdev, dma2, size,
				       DMA_BIDIRECTIONAL);

	/* MAP FMR */
	page_list = vmalloc(sizeof(u64));
	page_list[0] = dma1;
	rc = ib_map_phys_fmr(fmr1, page_list, 1, (u64)buf1);
	if (rc) {
		error("ib_map_phys_fmr 1\n");
		return -1;
	}
	page_list[0] = dma2;
	rc = ib_map_phys_fmr(fmr2, page_list, 1, (u64)buf2);
	if (rc) {
		error("ib_map_phys_fmr 2\n");
		return -1;
	}
	vfree(page_list);

	/* CQ */
	cq = ib_create_cq(test_ctx->ibdev, NULL, NULL, NULL, 4, 0);
	if (!cq || IS_ERR(cq)) {
		error("ib_create_cq\n");
		return -1;
	}

	/* QP */
	memset(&qp_init_attr, 0, sizeof(struct ib_qp_init_attr));
	qp_init_attr.send_cq = cq;
	qp_init_attr.recv_cq = cq;
	qp_init_attr.cap.max_send_wr = 4;
	qp_init_attr.cap.max_recv_wr = 4;
	qp_init_attr.cap.max_send_sge = 1;
	qp_init_attr.cap.max_recv_sge = 1;
	qp_init_attr.sq_sig_type = IB_SIGNAL_ALL_WR;
	qp_init_attr.qp_type = IB_QPT_RC;
	qp = ib_create_qp(test_ctx->pd, &qp_init_attr);
	if (!qp || IS_ERR(qp)) {
		error("ib_create_cq\n");
		return -1;
	}
	port = 1;
	rc = ib_query_port(test_ctx->ibdev, port, &port_attr);	
	if (rc) {
		error("ib_query_port\n");
		return -1;
	}
	rc = prep_qp_to_com(qp, port, port_attr.lid, qp->qp_num);
	if (rc) {
		error("prep_qp_to_com\n");
		return -1;
	}

	/* RDMA */
	rc = rdma_send(qp, fmr1->lkey, (u64)buf1, size, fmr2->rkey, (u64)buf2);
	if (rc) {
		error("rdma_send\n");
		return -1;
	}

	/* Check result */
	ib_dma_sync_single_for_cpu(test_ctx->ibdev, dma2, size,
				   DMA_BIDIRECTIONAL);
	info("[%s] buf2=%s\n", test_ctx->ibdev->name, buf2);
	ib_dma_sync_single_for_device(test_ctx->ibdev, dma2, size,
				      DMA_BIDIRECTIONAL);

	/* Cleanup */
	ib_destroy_qp(qp);
	ib_destroy_cq(cq);
	{
		LIST_HEAD(fmr_list);
		list_add(&fmr1->list, &fmr_list);
		rc = ib_unmap_fmr(&fmr_list);
		if (rc)
			error("ib_unmap_fmr 1\n");
	}
	{
		LIST_HEAD(fmr_list);
		list_add(&fmr2->list, &fmr_list);
		rc = ib_unmap_fmr(&fmr_list);
		if (rc)
			error("ib_unmap_fmr 2\n");
	}
	ib_dealloc_fmr(fmr1);
	ib_dealloc_fmr(fmr2);
	ib_dma_unmap_single(test_ctx->ibdev, dma1, size, DMA_BIDIRECTIONAL);
	ib_dma_unmap_single(test_ctx->ibdev, dma1, size, DMA_BIDIRECTIONAL);
	kfree(buf1);
	kfree(buf2);

	return 0;
}
