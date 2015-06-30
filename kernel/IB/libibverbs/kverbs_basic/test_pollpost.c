/*
 * Copyright (c) 2005 Mellanox Technologies. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * $Id: test_pollpost.c 13708 2011-08-25 12:25:23Z saeedm $ 
 * 
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h>       /* for register char device */
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <vl_ib_verbs.h>

#include <vl.h>
#include "my_types.h"
#include "func_headers.h"

struct send_wr_list {
	struct list_head 	list;
	struct ib_send_wr	wr;
};

struct recv_wr_list {
	struct list_head 	list;
	struct ib_recv_wr	wr;
};

// DMA Memory resources
struct dma_mr_t{
	struct ib_mr *mr;
	char *buf;
    dma_addr_t dma;
    struct ib_pd *pd;
	int size;
};


u32 basic_ib_local_dma_lkey(        
	IN	struct resources *res,
	IN	struct ib_mr *mr)
{
	return (res->dma_local_lkey ? res->device->local_dma_lkey : mr->lkey);
}

int post_recv(
	IN	struct resources *res,
	IN	struct ib_qp *qp,
	IN	struct ib_mr *mr,
	IN	dma_addr_t addr,
	IN	int size)
{
	struct ib_sge list = {
		.addr 	= addr,
		.length = size,
		.lkey 	= basic_ib_local_dma_lkey(res, mr),
	};
	struct ib_recv_wr wr = {
		.wr_id 	    = 0,
		.sg_list    = &list,
		.num_sge    = 1,
	};
	struct ib_recv_wr *bad_wr;
	int rc;

	rc = ib_post_recv(qp, &wr, &bad_wr);
	CHECK_VALUE("ib_post_recv", rc, 0, return rc);

	return 0;
}

int post_send(
	IN	struct resources *res,
	IN	struct ib_qp *qp,
	IN	struct ib_mr *mr,
	IN	dma_addr_t addr,
	IN	int size)
{
	struct ib_sge list = {
		.addr 	= addr,
		.length = size,
		.lkey 	= basic_ib_local_dma_lkey(res, mr),
	};
	struct ib_send_wr wr = {
		.wr_id 	    = 1,
		.sg_list    = &list,
		.num_sge    = 1,
		.opcode     = IB_WR_SEND,
		.send_flags = IB_SEND_SIGNALED,
	};
	struct ib_send_wr *bad_wr;
	int rc;

	rc = ib_post_send(qp, &wr, &bad_wr);
	CHECK_VALUE("ib_post_send", rc, 0, ;);

	return rc;
}

void free_dma_memory_resources(struct resources *res,struct dma_mr_t* dma_mr)
{
    if (!IS_ERR(dma_mr->mr) && dma_mr->mr) {
        if (ib_dereg_mr(dma_mr->mr))
            printk("ib_dereg_mr failed\n");
        dma_mr->mr = ERR_PTR(-EINVAL);
    }

    if (!IS_ERR(dma_mr->pd) && dma_mr->pd) {
        if (ib_dealloc_pd(dma_mr->pd))
            printk("ib_dealloc_pd failed\n");
        dma_mr->pd = ERR_PTR(-EINVAL);
    }

    if (dma_mr->buf) {
        ib_dma_unmap_single(res->device, dma_mr->dma, dma_mr->size, DMA_BIDIRECTIONAL);
        kfree(dma_mr->buf);
    }
    memset(dma_mr,0,sizeof(*dma_mr));

}

int create_dma_memory_resource(struct resources *res,struct dma_mr_t* dma_mr,int size)
{

    dma_mr->size = size;
    dma_mr->buf = kmalloc( dma_mr->size, GFP_DMA); 
    if (!dma_mr->buf) {
        printk("my_kmalloc failed\n");
        goto cleanup;
    }

    dma_mr->dma = ib_dma_map_single(res->device, dma_mr->buf,  dma_mr->size, DMA_BIDIRECTIONAL);
    if (ib_dma_mapping_error(res->device,dma_mr->dma)) {
        printk("ib_dma_map_single failed\n");
        goto cleanup;
    }

    dma_mr->pd = ib_alloc_pd(res->device);
    if (IS_ERR(dma_mr->pd)) {
        printk("ib_alloc_pd failed\n");
        goto cleanup;
    }

    //TODO : check if pd is allocated . 
    dma_mr->mr = ib_get_dma_mr(dma_mr->pd, IB_ACCESS_LOCAL_WRITE | IB_ACCESS_REMOTE_WRITE | IB_ACCESS_REMOTE_READ);
    if (IS_ERR(dma_mr->mr)) {
        printk("ib_get_dma_mr failed\n");
        goto cleanup;
    }

    return 0;
cleanup:
    free_dma_memory_resources(res,dma_mr);
    return -1;

}


int my_create_sr_desc(
     IN	struct resources *res,
	IN	struct VL_random_t *rand_gen,
	IN	enum ib_wr_opcode opcode,
	IN	enum ib_send_flags flags,
	IN	int num_sge,
	IN	int num_wr,
	IN	struct ib_ah *av,
	IN 	struct dma_mr_t *dma_mr,
	OUT	struct list_head *wr_head)
{
	struct send_wr_list	*wr_node = NULL;
	struct send_wr_list	*temp = NULL;
	struct ib_send_wr	*last = NULL;
	struct ib_sge		*sg_list = NULL;
	int			i;
	int length ;


	INIT_LIST_HEAD(wr_head);

	sg_list = kmalloc(num_sge * sizeof(struct ib_sge), GFP_KERNEL);


	CHECK_PTR("kmalloc", sg_list, goto error);

	memset(sg_list, 0, num_sge * sizeof(*sg_list));




    length = VL_range(rand_gen,1,MAX_LENGTH);

    if(flags == 3 /*IB_SEND_INLINE*/){
    	length = VL_range(rand_gen,1,res->attributes.qp_init_attr.cap.max_inline_data/num_sge);
    }
    if ((opcode == IB_WR_ATOMIC_CMP_AND_SWP) || (opcode == IB_WR_ATOMIC_FETCH_AND_ADD))
           length = 8;
    if(create_dma_memory_resource(res,dma_mr,length))
    {
        printk("create DMA memory Failed\n");
        goto error;
    }

	for (i = 0 ; i < num_sge; ++i) {
                sg_list[i].addr = (u64) dma_mr->dma;
                if(flags == 3 /*IB_SEND_INLINE*/)
                	sg_list[i].addr = (u64) dma_mr->buf;
                sg_list[i].length = dma_mr->size;
                sg_list[i].lkey = dma_mr->mr->lkey;
	}

	for (i = 0; i < num_wr; ++i) {
		wr_node = kmalloc(sizeof(struct send_wr_list), GFP_KERNEL);
		CHECK_PTR("kmalloc", wr_node, goto error);

		wr_node->wr.ex.imm_data = VL_random(rand_gen, 0xFFFFFFFF);
		wr_node->wr.num_sge = num_sge;
		wr_node->wr.sg_list = sg_list;
		wr_node->wr.opcode = opcode;
		wr_node->wr.send_flags = 1 << flags;
		wr_node->wr.wr_id = VL_random64(rand_gen, 0xFFFFFFFFFFFFFFFFULL);

		switch (opcode) {
		case IB_WR_RDMA_WRITE:
		case IB_WR_RDMA_WRITE_WITH_IMM:
			wr_node->wr.wr.rdma.remote_addr = VL_random64(rand_gen, 0xFFFFFFFFFFFFFFFFULL);
			wr_node->wr.wr.rdma.rkey = VL_random(rand_gen, 0xFFFFFFFF);
			break;
		case IB_WR_SEND:
		case IB_WR_SEND_WITH_IMM:
			wr_node->wr.wr.ud.ah = av;
			wr_node->wr.wr.ud.remote_qpn = VL_random(rand_gen, 0xFFFFFFFF);
			wr_node->wr.wr.ud.remote_qkey = VL_random(rand_gen, 0xFFFFFFFF);
			break;
		case IB_WR_ATOMIC_CMP_AND_SWP:
		case IB_WR_ATOMIC_FETCH_AND_ADD:
			wr_node->wr.wr.atomic.remote_addr = VL_random64(rand_gen, 0xFFFFFFFFFFFFFFFFULL);
			wr_node->wr.wr.atomic.compare_add = VL_random64(rand_gen, 0xFFFFFFFFFFFFFFFFULL);
			wr_node->wr.wr.atomic.swap = VL_random64(rand_gen, 0xFFFFFFFFFFFFFFFFULL);
			wr_node->wr.wr.atomic.rkey = VL_random(rand_gen, 0xFFFFFFFF);
			break;
		default:
			break;
		}

		list_add_tail(&wr_node->list, wr_head);
	}

	list_for_each_entry_reverse(wr_node, wr_head, list) {
		wr_node->wr.next = last;
		last = &wr_node->wr;
	}

	return 0;

error:
    	free_dma_memory_resources(res, dma_mr);
	if (sg_list)
		kfree(sg_list);

	list_for_each_entry_safe(wr_node, temp, wr_head, list) {
		kfree(wr_node);
	}

	return -1;
}

int my_create_rr_desc(
	IN	struct VL_random_t *rand_gen,
	IN	int num_sge,
	IN	int num_wr,
	OUT	struct list_head *wr_head)
{
	struct ib_sge 		*sg_list = NULL;
	struct recv_wr_list	*wr_node = NULL;
	struct recv_wr_list	*temp = NULL;
	struct ib_recv_wr	*last = NULL;
	int			i;

	INIT_LIST_HEAD(wr_head);

	sg_list = kmalloc(num_sge * sizeof(struct ib_sge), GFP_KERNEL);
	CHECK_PTR("kmalloc", sg_list, goto error);

	memset(sg_list, 0, num_sge * sizeof(*sg_list));
	for (i = 0 ; i < num_sge; ++i) {
		sg_list[i].addr = VL_random64(rand_gen, 0xFFFFFFFFFFFFFFFFULL);
                sg_list[i].length = VL_random(rand_gen, 0xFFFFFFFF);
                sg_list[i].lkey = VL_random(rand_gen, 0xFFFFFFFF);
	}

	for (i = 0; i < num_wr; ++i) {
		wr_node = kmalloc(sizeof(struct recv_wr_list), GFP_KERNEL);
		CHECK_PTR("kmalloc", wr_node, goto error);

		wr_node->wr.num_sge = num_sge;
		wr_node->wr.sg_list = sg_list;
		wr_node->wr.wr_id = VL_random64(rand_gen, 0xFFFFFFFFFFFFFFFFULL);

		list_add_tail(&wr_node->list, wr_head);
	}

	list_for_each_entry_reverse(wr_node, wr_head, list) {
		wr_node->wr.next = last;
		last = &wr_node->wr;
	}

	return 0;

error:
	if (sg_list)
		kfree(sg_list);

	list_for_each_entry_safe(wr_node, temp, wr_head, list) {
		kfree(wr_node);
	}

	return -1;
}

/* ib_post_send */
int send_1(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_pd		*pd = ERR_PTR(-EINVAL);
	struct ib_ah		*av = ERR_PTR(-EINVAL);
	struct ib_cq	        *cq = ERR_PTR(-EINVAL);
	struct ib_qp		*qp = ERR_PTR(-EINVAL);
	struct list_head 	wr_head;
	struct send_wr_list	*wr_node = NULL;
	struct send_wr_list	*temp = NULL;
	struct dma_mr_t		*dma_mr = NULL;
	int			size;
	int			sig_type;
	int			qp_type;
	int			opcode;
	int			send_flags;
	int			i;
	int			list_clean = 1;
	int                     rc;
	int                     result = -1;

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	size = VL_range(&res->rand_g, 1, GET_MAX_CQE(res) / 2); 
	cq = ib_create_cq(res->device, cq_comp_handler, component_event_handler, NULL, size, 0);
	CHECK_PTR("ib_create_cq", !IS_ERR(cq), goto cleanup);

	{
		struct ib_ah_attr 	attr;

		create_random_av_attr(res, &attr);

		av = ib_create_ah(pd, &attr);
		CHECK_PTR("ib_create_ah", !IS_ERR(av), goto cleanup);
	}

	dma_mr = kmalloc(sizeof(struct dma_mr_t), GFP_KERNEL);
	CHECK_PTR("kmalloc", dma_mr, goto cleanup);
	memset(dma_mr,0,sizeof(*dma_mr));

	for (sig_type = IB_SIGNAL_ALL_WR; sig_type <= IB_SIGNAL_REQ_WR; sig_type++) {
		for (qp_type = IB_QPT_RC; qp_type <= IB_QPT_UD; qp_type++) {
			for (opcode = 0; opcode < 7; opcode++) {
				for (send_flags = 0; send_flags < 4; send_flags++) {
					struct ib_send_wr *bad_wr;
					struct ib_send_wr *wr;
					int num_sge;
					int num_wr;

					VL_MISC_TRACE1(("QP %d sig_type %d opcode %d send_flags %d\n", qp_type, sig_type, opcode, send_flags));
					switch (qp_type) {
					case 2: 
						TEST_CASE(("post sr to RC QP"));
						break;
					case 3:
						if (opcode == IB_WR_RDMA_READ || 
						    opcode == IB_WR_ATOMIC_CMP_AND_SWP || 
						    opcode == IB_WR_ATOMIC_FETCH_AND_ADD) {
							continue;
						}
						TEST_CASE(("post sr to UC QP"));
						break;
					case 4:
						if (opcode != IB_WR_SEND && 
						    opcode != IB_WR_SEND_WITH_IMM) {
							continue;
						}
						TEST_CASE(("post sr to UD QP"));
						break;
					}
					schedule();

					memset(&res->attributes.qp_init_attr, 0, sizeof(res->attributes.qp_init_attr));
					init_qp_cap(res, &res->attributes.qp_init_attr.cap);
					res->attributes.qp_init_attr.recv_cq = cq;
					res->attributes.qp_init_attr.send_cq = cq;
					res->attributes.qp_init_attr.qp_type = qp_type;
					res->attributes.qp_init_attr.sq_sig_type = sig_type;
					res->attributes.qp_init_attr.event_handler = component_event_handler;

					if(send_flags == 3 /*IB_SEND_INLINE*/){
						res->attributes.qp_init_attr.cap.max_inline_data = VL_range(&res->rand_g, 1, INLINE_THRESHOLD);
						//if ATOMIC op then max inline data should be at least 8.
						if ((opcode == IB_WR_ATOMIC_CMP_AND_SWP) || (opcode == IB_WR_ATOMIC_FETCH_AND_ADD))
							res->attributes.qp_init_attr.cap.max_inline_data = VL_range(&res->rand_g, 8, INLINE_THRESHOLD);
						VL_MISC_TRACE1(("requesting :max inline = %d max sge = %d\n",res->attributes.qp_init_attr.cap.max_inline_data,res->attributes.qp_init_attr.cap.max_send_sge));

					}

					qp = ib_create_qp(pd, &res->attributes.qp_init_attr);
					CHECK_PTR("ib_create_qp", !IS_ERR(qp), VL_DATA_ERR(("error %ld",PTR_ERR(qp)));  goto cleanup);
					VL_MISC_TRACE1(("allocated max inline = %d max sge = %d\n",res->attributes.qp_init_attr.cap.max_inline_data,res->attributes.qp_init_attr.cap.max_send_sge));

					rc = my_modify_qp(&res->rand_g, qp, qp_type, VL_range(&res->rand_g, 1, res->device->phys_port_cnt), IB_QPS_RTS, 0);
					CHECK_VALUE("my_modify_qp", rc, 0, goto cleanup);

					if (opcode == IB_WR_ATOMIC_CMP_AND_SWP || opcode == IB_WR_ATOMIC_FETCH_AND_ADD)
						num_sge = 1;
					else
						num_sge = VL_range(&res->rand_g, 1, res->attributes.qp_init_attr.cap.max_send_sge);

					num_wr = VL_range(&res->rand_g, 1, res->attributes.qp_init_attr.cap.max_send_wr);

					rc = my_create_sr_desc(res,&res->rand_g, opcode, send_flags, num_sge, 
										   num_wr, (qp_type == IB_QPT_UD) ? av : NULL, dma_mr, &wr_head);
					CHECK_VALUE("my_create_sr_desc", rc, 0, goto cleanup);

					list_clean = 0;

					wr = &((struct send_wr_list *)wr_head.next)->wr;

					for (i = 0; i < res->attributes.qp_init_attr.cap.max_send_wr; i += num_wr) {
						if (i + num_wr > res->attributes.qp_init_attr.cap.max_send_wr)
							break;

						rc = ib_post_send(qp, wr, &bad_wr);
						CHECK_VALUE("ib_post_send", rc, 0, goto cleanup);
					}
					

					num_wr = res->attributes.qp_init_attr.cap.max_send_wr - 
						(res->attributes.qp_init_attr.cap.max_send_wr / num_wr) * num_wr;

					i = 0;
					list_for_each_entry_safe(wr_node, temp, &wr_head, list) {
						if (i == 0) {
							kfree(wr_node->wr.sg_list);
							i++;
						}
						kfree(wr_node);
					}

					free_dma_memory_resources(res,dma_mr);
					list_clean = 1;

					if (num_wr) {
						rc = my_create_sr_desc(res,&res->rand_g, opcode, send_flags, num_sge, 
										   num_wr, (qp_type == IB_QPT_UD) ? av : NULL,dma_mr, &wr_head);
						CHECK_VALUE("my_create_sr_desc", rc, 0, goto cleanup);
						list_clean = 0;
	
						wr = &((struct send_wr_list *)wr_head.next)->wr;
						rc = ib_post_send(qp, wr, &bad_wr);
						CHECK_VALUE("ib_post_send", rc, 0, goto cleanup);
	
						i = 0;
						list_for_each_entry_safe(wr_node, temp, &wr_head, list) {
							if (i == 0) {
								kfree(wr_node->wr.sg_list);
								i++;
							}
							kfree(wr_node);
						}

						free_dma_memory_resources(res,dma_mr);
						list_clean = 1;
					}
                                        
					rc = ib_destroy_qp(qp);
					CHECK_VALUE("ib_destroy_qp", rc, 0, goto cleanup);
					qp = ERR_PTR(-EINVAL);

					PASSED;
				}
			}
		}
	}

	result = 0;

cleanup:

	i = 0;
        if (!list_clean) {
		list_for_each_entry_safe(wr_node, temp, &wr_head, list) {
			if (i == 0) {
				kfree(wr_node->wr.sg_list);
				i++;
			}
			kfree(wr_node);
		}
		free_dma_memory_resources(res,dma_mr);
	}
        if(dma_mr){
        	kfree(dma_mr);
        }

	if (!IS_ERR(qp) && qp) {
		rc = ib_destroy_qp(qp);
		CHECK_VALUE("ib_destroy_qp", rc, 0, result = -1);
	}

	if (!IS_ERR(av)) {
		rc = ib_destroy_ah(av);
		CHECK_VALUE("ib_destroy_ah", rc, 0, result = -1);
	}

	if (!IS_ERR(cq)) {
		rc = ib_destroy_cq(cq);
		CHECK_VALUE("ib_destroy_cq", rc, 0, result = -1);
	}

	if (!IS_ERR(pd)) {
		rc = ib_dealloc_pd(pd);
		CHECK_VALUE("ib_dealloc_pd", rc, 0, result = -1);
	}

	return result;
}

/* ib_post_recv */
int recv_1(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_pd		*pd = ERR_PTR(-EINVAL);
	struct ib_cq           	*cq = ERR_PTR(-EINVAL);
	struct ib_qp		*qp = ERR_PTR(-EINVAL);
	struct list_head 	wr_head;
	struct recv_wr_list	*wr_node = NULL;
	struct recv_wr_list	*temp = NULL;
	int			size;
	int			sig_type;
	int			qp_type;
	enum ib_qp_state	state;
	int			list_clean = 1;
	int			i;
	int                     rc;
	int                     result = -1;

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	size = GET_MAX_CQE(res); 
	cq = ib_create_cq(res->device, cq_comp_handler, component_event_handler, NULL, size, 0);
	CHECK_PTR("ib_create_cq", !IS_ERR(cq), goto cleanup);

	for (sig_type = IB_SIGNAL_ALL_WR; sig_type <= IB_SIGNAL_REQ_WR; sig_type++) {
		for (qp_type = 2; qp_type < 5; qp_type++) {
			for (state = IB_QPS_RTR; state <= IB_QPS_RTS; state++) {
				struct ib_recv_wr *bad_wr;
				struct ib_recv_wr *wr;
				int num_sge;
				int num_wr;


				switch (qp_type) {
				case IB_QPT_RC:
					TEST_CASE(("post RR to RC QP"));
					break;
				case IB_QPT_UC:
					TEST_CASE(("post RR to UC QP"));
					break;
				case IB_QPT_UD:
					TEST_CASE(("post RR to UD QP"));
					break;
				}

				memset(&res->attributes.qp_init_attr, 0, sizeof(res->attributes.qp_init_attr));
				init_qp_cap(res, &res->attributes.qp_init_attr.cap);
				res->attributes.qp_init_attr.recv_cq = cq;
				res->attributes.qp_init_attr.send_cq = cq;
				res->attributes.qp_init_attr.qp_type = qp_type;
				res->attributes.qp_init_attr.sq_sig_type = sig_type;
				res->attributes.qp_init_attr.event_handler = component_event_handler;
			
				qp = ib_create_qp(pd, &res->attributes.qp_init_attr);
				CHECK_PTR("ib_create_qp", !IS_ERR(qp), VL_DATA_ERR(("error %ld",PTR_ERR(qp)));  goto cleanup);

				rc = my_modify_qp(&res->rand_g, qp, qp_type, VL_range(&res->rand_g, 1, res->device->phys_port_cnt), state, 0);
				CHECK_VALUE("my_modify_qp", rc, 0, goto cleanup);

				num_sge = VL_range(&res->rand_g, 1, res->attributes.qp_init_attr.cap.max_recv_sge);
				num_wr = VL_range(&res->rand_g, 1, res->attributes.qp_init_attr.cap.max_recv_wr);

				rc = my_create_rr_desc(&res->rand_g, num_sge, num_wr, &wr_head);
				CHECK_VALUE("my_create_rr_desc", rc, 0, goto cleanup);

				list_clean = 0;

				wr = &((struct recv_wr_list *)wr_head.next)->wr;
	
				for (i = 0; i < res->attributes.qp_init_attr.cap.max_recv_wr; i += num_wr) {
					if (i + num_wr > res->attributes.qp_init_attr.cap.max_recv_wr)
						break;
	
					rc = ib_post_recv(qp, wr, &bad_wr);
					CHECK_VALUE("ib_post_recv", rc, 0, goto cleanup);
				}

				num_wr = res->attributes.qp_init_attr.cap.max_recv_wr - 
					(res->attributes.qp_init_attr.cap.max_recv_wr / num_wr) * num_wr;

				i = 0;
				list_for_each_entry_safe(wr_node, temp, &wr_head, list) {
					if (i == 0) {
						kfree(wr_node->wr.sg_list);
						i++;
					}
					kfree(wr_node);
				}
				list_clean = 1;

				if (num_wr) {
					rc = my_create_rr_desc(&res->rand_g, num_sge, num_wr, &wr_head);
					CHECK_VALUE("my_create_rr_desc", rc, 0, goto cleanup);
					list_clean = 0;

					wr = &((struct recv_wr_list *)wr_head.next)->wr;
					rc = ib_post_recv(qp, wr, &bad_wr);
					CHECK_VALUE("ib_post_recv", rc, 0, goto cleanup);

					i = 0;
					list_for_each_entry_safe(wr_node, temp, &wr_head, list) {
						if (i == 0) {
							kfree(wr_node->wr.sg_list);
							i++;
						}
						kfree(wr_node);
					}
					list_clean = 1;
				}
				
				rc = ib_destroy_qp(qp);
				CHECK_VALUE("ib_destroy_qp", rc, 0, goto cleanup);
				qp = ERR_PTR(-EINVAL);
	
				PASSED;
			}
		}
	}

	result = 0;

cleanup:

	i = 0;
        if (!list_clean) {
		list_for_each_entry_safe(wr_node, temp, &wr_head, list) {
			if (i == 0) {
				kfree(wr_node->wr.sg_list);
				i++;
			}
			kfree(wr_node);
		}
	}

	if (!IS_ERR(qp) && qp) {
		rc = ib_destroy_qp(qp);
		CHECK_VALUE("ib_destroy_qp", rc, 0, result = -1);
	}

	if (!IS_ERR(cq)) {
		rc = ib_destroy_cq(cq);
		CHECK_VALUE("ib_destroy_cq", rc, 0, result = -1);
	}

	if (!IS_ERR(pd)) {
		rc = ib_dealloc_pd(pd);
		CHECK_VALUE("ib_dealloc_pd", rc, 0, result = -1);
	}

	return result;
}

/* ib_post_srq_recv */
int recv_2(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_pd		*pd = ERR_PTR(-EINVAL);
	struct ib_cq           	*cq = ERR_PTR(-EINVAL);
	struct ib_srq		*srq = ERR_PTR(-EINVAL);
	struct list_head 	wr_head;
	struct recv_wr_list	*wr_node = NULL;
	struct recv_wr_list	*temp = NULL;
	int			size;
	struct ib_recv_wr	*wr;
	struct ib_recv_wr 	*bad_wr;
	int 			num_sge;
	int 			num_wr;
	int			list_clean = 1;
	int			i;
	int                     rc;
	int                     result = -1;

	TEST_CASE("post RR to SRQ");

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	size = GET_MAX_CQE(res); 
	cq = ib_create_cq(res->device, cq_comp_handler, component_event_handler, NULL, size, 0);
	CHECK_PTR("ib_create_cq", !IS_ERR(cq), goto cleanup);

	memset(&res->attributes.srq_init_attr, 0, sizeof(res->attributes.srq_init_attr));
	res->attributes.srq_init_attr.event_handler = component_event_handler;
	init_srq_cap(config, res, &res->attributes.srq_init_attr);

	srq = ib_create_srq(pd, &res->attributes.srq_init_attr);
	CHECK_PTR("ib_create_srq", !IS_ERR(srq), goto cleanup);

	num_sge = VL_range(&res->rand_g, 1, res->attributes.srq_init_attr.attr.max_sge);
	num_wr = VL_range(&res->rand_g, 1, res->attributes.srq_init_attr.attr.max_wr);

	rc = my_create_rr_desc(&res->rand_g, num_sge, num_wr, &wr_head);
	CHECK_VALUE("my_create_rr_desc", rc, 0, goto cleanup);
	list_clean = 0;

	wr = &((struct recv_wr_list *)wr_head.next)->wr;
	for (i = 0; i < res->attributes.srq_init_attr.attr.max_wr; i += num_wr) {
		if (i + num_wr > res->attributes.srq_init_attr.attr.max_wr)
			break;

		rc = ib_post_srq_recv(srq, wr, &bad_wr);
		CHECK_VALUE("ib_post_srq_recv", rc, 0, goto cleanup);
	}

	num_wr = res->attributes.qp_init_attr.cap.max_recv_wr - (res->attributes.qp_init_attr.cap.max_recv_wr / num_wr) * num_wr;

	i = 0;
	list_for_each_entry_safe(wr_node, temp, &wr_head, list) {
		if (i == 0) {
			kfree(wr_node->wr.sg_list);
			i++;
		}
		kfree(wr_node);
	}
	list_clean = 1;

	PASSED;

	result = 0;

cleanup:
        
	i = 0;
        if (!list_clean) {
		list_for_each_entry_safe(wr_node, temp, &wr_head, list) {
			if (i == 0) {
				kfree(wr_node->wr.sg_list);
				i++;
			}
			kfree(wr_node);
		}
	}

	if (!IS_ERR(srq) && srq) {
		rc = ib_destroy_srq(srq);
		CHECK_VALUE("ib_destroy_srq", rc, 0, result = -1);
	}

	if (!IS_ERR(cq)) {
		rc = ib_destroy_cq(cq);
		CHECK_VALUE("ib_destroy_cq", rc, 0, result = -1);
	}

	if (!IS_ERR(pd)) {
		rc = ib_dealloc_pd(pd);
		CHECK_VALUE("ib_dealloc_pd", rc, 0, result = -1);
	}

	return result;
}

/* ib_poll_cq */
int poll_1(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_pd 		*pd = ERR_PTR(-EINVAL);
	struct ib_cq		*cq = ERR_PTR(-EINVAL);
	struct ib_mr		*mr = ERR_PTR(-EINVAL);
	dma_addr_t		dma = 0;
	char			*buf = NULL;
	struct qp_list_node	*qp_node = NULL;
	struct qp_list_node	*temp = NULL;
	struct list_head	qp_list_head;
	struct ib_wc		*wc_array = NULL;
	int			num_post;
	int			num_qp;
	int			cqe;
	int			num_poll;
	int			i;
	int			j;
	int			result = -1;
	int			rc;

	TEST_CASE(("Post CQEs and poll one at a time"));

	schedule();

	INIT_LIST_HEAD(&qp_list_head);

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	cqe = VL_range(&res->rand_g, 1, GET_MAX_CQE(res)); 

	cq = ib_create_cq(res->device, cq_comp_handler, component_event_handler, NULL, cqe, 0);
	CHECK_PTR("ib_create_cq", !IS_ERR(cq), goto cleanup);

	num_qp = cq->cqe / 100 + 1;

	memset(&res->attributes.qp_init_attr, 0, sizeof(res->attributes.qp_init_attr));
	init_qp_cap(res, &res->attributes.qp_init_attr.cap);
	res->attributes.qp_init_attr.recv_cq = cq;
	res->attributes.qp_init_attr.send_cq = cq;
	res->attributes.qp_init_attr.qp_type = IB_QPT_UC;
	res->attributes.qp_init_attr.cap.max_recv_wr  = 1;
	res->attributes.qp_init_attr.cap.max_send_sge = 1;
	res->attributes.qp_init_attr.cap.max_recv_sge = 1;
	res->attributes.qp_init_attr.event_handler = component_event_handler;
	res->attributes.qp_init_attr.sq_sig_type = 1;

	for (i = 0 ; i < num_qp; ++i) {
		qp_node = kmalloc(sizeof(*qp_node), GFP_KERNEL);
		CHECK_PTR("kmalloc", qp_node, goto cleanup);

		qp_node->qp = ib_create_qp(pd, &res->attributes.qp_init_attr);
		CHECK_PTR("ib_create_qp", !IS_ERR(qp_node->qp), VL_DATA_ERR(("error %ld",PTR_ERR(qp_node->qp)));  goto cleanup);

		list_add_tail(&qp_node->list, &qp_list_head);

		rc = my_modify_qp(&res->rand_g, qp_node->qp, IB_QPT_UC, VL_range(&res->rand_g, 1, res->device->phys_port_cnt), IB_QPS_RTS, 0);
		CHECK_VALUE("my_modify_qp", rc, 0, goto cleanup);
	}

        buf = kmalloc(1, GFP_DMA);
	CHECK_PTR("kmalloc", buf, goto cleanup);

	dma = ib_dma_map_single(res->device, buf, 1, DMA_BIDIRECTIONAL);
	CHECK_VALUE("ib_dma_map_single", ib_dma_mapping_error(res->device, dma), 0, goto cleanup);

	if (!res->dma_local_lkey) {
		mr = ib_get_dma_mr(pd, IB_ACCESS_LOCAL_WRITE);
		CHECK_PTR("ib_get_dma_mr", !IS_ERR(mr), goto cleanup);
	} else {
		VL_MISC_TRACE1(("Use RESERVED LKEY\n"));
		mr = ERR_PTR(-EINVAL);
	}

	num_post = VL_range(&res->rand_g, 1, res->attributes.qp_init_attr.cap.max_send_wr);
	if (num_post > cq->cqe)
		num_post = cq->cqe;

	num_post = (num_post / num_qp) * num_qp;

	list_for_each_entry(qp_node, &qp_list_head, list) {
		for (i = 0; i < num_post / num_qp; ++i) {
			rc = post_send(res, qp_node->qp, mr, dma, 1);
			CHECK_VALUE("post_send", rc, 0, goto cleanup);
		}
	}

	for (i = 0; i < num_post; ++i) {
		struct ib_wc wc;
		int timeout = 0;

		do {
			rc = ib_poll_cq(cq, 1, &wc);
			if (rc == 0) {
				timeout++;
				msleep(10);
			}
		} while (rc == 0 && timeout < 200);
		if (timeout == 200) {
			FAILED;
			VL_DATA_ERR(("ib_poll_cq timeout i = %d", i));
			goto cleanup;
		}
		CHECK_VALUE("ib_poll_cq", rc, 1, goto cleanup);
		CHECK_VALUE("wc status", wc.status, IB_WC_SUCCESS, goto cleanup);
	}

	PASSED;

	TEST_CASE("Post CQEs and poll some at a time");

	list_for_each_entry(qp_node, &qp_list_head, list) {
		for (i = 0; i < num_post / num_qp; ++i) {
			rc = post_send(res, qp_node->qp, mr, dma, 1);
			CHECK_VALUE("post_send", rc, 0, goto cleanup);
		}
	}

	num_poll = VL_range(&res->rand_g, 2, 10);

	wc_array = kmalloc(num_poll * sizeof(*wc_array), GFP_KERNEL);
	CHECK_PTR("kmalloc", wc_array, goto cleanup);

	for (i = 0; i < num_post; ) {
		int timeout = 0;

		do {
			rc = ib_poll_cq(cq, num_poll, wc_array);
			if (rc == 0) {
				timeout++;
				msleep(10);
			}
		} while (rc == 0 && timeout < 200);
		if (timeout == 200) {
			FAILED;
			VL_DATA_ERR(("ib_poll_cq timeout i = %d", i));
			goto cleanup;
		}
		/* check the the poll didn't fail */
		if (rc < 0) {
			FAILED;
			VL_DATA_ERR(("Error in %s, value is %d",
				("ib_poll_cq"), rc));
			goto cleanup;
		}
		i += rc;

		for (j = 0; j < rc; ++j) {
			CHECK_VALUE("wc status", wc_array[j].status, IB_WC_SUCCESS, goto cleanup);
		}
	}

	PASSED;

	result = 0;

cleanup:

	list_for_each_entry_safe(qp_node, temp, &qp_list_head, list) {
		rc = ib_destroy_qp(qp_node->qp);
		CHECK_VALUE("ib_destroy_qp", rc, 0, return -1);

		kfree(qp_node);
	}

	if (!IS_ERR(cq)) {
		rc = ib_destroy_cq(cq);
		CHECK_VALUE("ib_destroy_cq", rc, 0, return -1);
	}

	if (!IS_ERR(mr)) {
		rc = ib_dereg_mr(mr);
		CHECK_VALUE("ib_dereg_mr", rc, 0, return -1);
	}

	if (!IS_ERR(pd)) {
		rc = ib_dealloc_pd(pd);
		CHECK_VALUE("ib_dealloc_pd", rc, 0, return -1);
	}

	if (buf) {
		ib_dma_unmap_single(res->device, dma, 1, DMA_BIDIRECTIONAL);
		kfree(buf);
	}

	if (wc_array)
		kfree(wc_array);

	return result;
}

int test_pollpost(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	RUN_TEST(1, send_1);
	RUN_TEST(2, recv_1);
	RUN_TEST(3, recv_2);
	RUN_TEST(4, poll_1);

	return 0;
}
