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
 * $Id: test_cq.c 10260 2010-11-07 15:34:41Z saeedm $ 
 * 
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h>       /* for register char device */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
        #include <asm/semaphore.h>      /* for semaphore */
#else
        #include <linux/semaphore.h>
#endif

#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/spinlock.h>

#include <vl_ib_verbs.h>

#include <vl.h>
#include "my_types.h"
#include "func_headers.h"

// TODO:
// 1 - enable ib_req_ncomp_notif


static struct semaphore thread_sync;
static int thread_res = -1;

static spinlock_t comp_sync;
static int num_comp = 0;


void cq_comp_handler(
	IN	struct ib_cq *cq, 
	IN	void *cq_context)
{
        spin_lock(&comp_sync);
	num_comp++;
	spin_unlock(&comp_sync);

	return;
}

/* ib_create_cq ib_destroy_cq */
int cq_1(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_cq 	*cq = ERR_PTR(-EINVAL);
	int		cqe;
	int		result = -1;
	int		rc;

	TEST_CASE("ib_create_cq ib_destroy_cq");

	cqe = VL_range(&res->rand_g, 1, GET_MAX_CQE(res));

    printk("\tcreate CQ with CQE =  %d ",cqe);

    cq = ib_create_cq(res->device, cq_comp_handler, component_event_handler, NULL, cqe, 0);
	CHECK_PTR("ib_create_cq", !IS_ERR(cq), goto cleanup);

	if (cq->cqe < cqe) {
		FAILED;
		VL_DATA_ERR(("wrong CQE size, expected at least %d actual %d", cqe, cq->cqe));
		goto cleanup;
	}

	rc = ib_destroy_cq(cq);
	CHECK_VALUE("ib_destroy_cq", rc, 0, goto cleanup);

	cq = ib_create_cq(res->device, cq_comp_handler, component_event_handler, component_event_handler, cqe, 0);
	CHECK_PTR("ib_create_cq", !IS_ERR(cq), goto cleanup);

	rc = ib_destroy_cq(cq);
	CHECK_VALUE("ib_destroy_cq", rc, 0, goto cleanup);

	PASSED;

	TEST_CASE("create CQ with size = TEST max CQE");

	cqe = GET_MAX_CQE(res);

    printk("\tcreate CQ with CQE =  %d ",cqe);

	cq = ib_create_cq(res->device, cq_comp_handler, component_event_handler, NULL, cqe, 0);
	CHECK_PTR("ib_create_cq", !IS_ERR(cq), goto cleanup);

	if (cq->cqe < cqe) {
		FAILED;
		VL_DATA_ERR(("wrong CQE size, expected at least %d actual %d", cqe, cq->cqe));
		goto cleanup;
	}

	rc = ib_destroy_cq(cq);
	CHECK_VALUE("ib_destroy_cq", rc, 0, goto cleanup);

	PASSED;

	TEST_CASE("create CQ with size = 0");

	cqe = 0;

	cq = ib_create_cq(res->device, cq_comp_handler, component_event_handler, NULL, cqe, 0);
	if (IS_ERR(cq)) {
		FAILED;
		VL_DATA_ERR(("CQ creation FAILED with size 0"));
		goto cleanup;
	}

	PASSED;

	TEST_CASE(("cqe > hca_cap.max_cqe"));

	cqe = res->device_attr.max_cqe + 1;

	cq = ib_create_cq(res->device, cq_comp_handler, component_event_handler, NULL, cqe, 0);
	if (!IS_ERR(cq)) {
		FAILED;
		VL_DATA_ERR(("CQ created with size larger than hca's cap"));
		goto cleanup;
	}

	cqe = VL_range(&res->rand_g, res->device_attr.max_cqe + 1, 0xFFFFFFFF);

	cq = ib_create_cq(res->device, cq_comp_handler, component_event_handler, NULL, cqe, 0);
	if (!IS_ERR(cq)) {
		FAILED;
		VL_DATA_ERR(("CQ created with size larger than hca's cap"));
		goto cleanup;
	}

        PASSED;

	result = 0;
cleanup:

	if (!IS_ERR(cq)) {
		rc = ib_destroy_cq(cq);
		CHECK_VALUE("ib_destroy_cq", rc, 0, result = -1);
	}

	return result;
}

/* ib_resize_cq */
int cq_2(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_cq 	*cq = ERR_PTR(-EINVAL);
	int		cqe, orig_cqe;
	int		result = -1;
	int		rc;

	TEST_CASE("ib_resize_cq to more than cqe");

	cqe = VL_range(&res->rand_g, 1, GET_MAX_CQE(res) / 2);
    printk("\tcreate CQ with CQE =  %d ",cqe);
	cq = ib_create_cq(res->device, cq_comp_handler, component_event_handler, NULL, cqe, 0);
	CHECK_PTR("ib_create_cq", !IS_ERR(cq), goto cleanup);

	cqe = VL_range(&res->rand_g, cq->cqe + 1,GET_MAX_CQE(res));
    printk("\tresize CQ to CQE =  %d ",cqe);
	rc = ib_resize_cq(cq, cqe);
	CHECK_VALUE("ib_resize_cq", rc, 0, goto cleanup);

	if (cq->cqe < cqe) {
		FAILED;
		VL_DATA_ERR(("wrong CQE size, expected at least %d actual %d", cqe, cq->cqe));
		goto cleanup;
	}

	PASSED;

	TEST_CASE("ib_resize_cq to less than cqe");

	cqe = VL_random(&res->rand_g, cq->cqe);

    printk("\tresize CQ to CQE =  %d ",cqe);

    rc = ib_resize_cq(cq, cqe);
	CHECK_VALUE("ib_resize_cq", rc, 0, goto cleanup);

	if (cq->cqe < cqe) {
		FAILED;
		VL_DATA_ERR(("wrong CQE size, expected at least %d actual %d", cqe, cq->cqe));
		goto cleanup;
	}

	PASSED;

	orig_cqe = cq->cqe;

	TEST_CASE("ib_resize_cq with size = 0");

	cqe = 0;

	rc = ib_resize_cq(cq, cqe);
	CHECK_VALUE("ib_resize_cq", rc, -EINVAL, goto cleanup);
	CHECK_VALUE("ib_resize_cq", cq->cqe, orig_cqe, goto cleanup);

	PASSED;

	TEST_CASE(("cqe > hca_cap.max_cqe"));

	cqe = res->device_attr.max_cqe + 1;

	rc = ib_resize_cq(cq, cqe);
	CHECK_VALUE("ib_resize_cq", rc, -EINVAL, goto cleanup);
	CHECK_VALUE("ib_resize_cq", cq->cqe, orig_cqe, goto cleanup);

	cqe = VL_range(&res->rand_g, res->device_attr.max_cqe + 1, 0xFFFFFFFF);

	rc = ib_resize_cq(cq, cqe);
	CHECK_VALUE("ib_resize_cq", rc, -EINVAL, goto cleanup);
	CHECK_VALUE("ib_resize_cq", cq->cqe, orig_cqe, goto cleanup);

        PASSED;

	result = 0;
cleanup:

	if (!IS_ERR(cq)) {
		rc = ib_destroy_cq(cq);
		CHECK_VALUE("ib_destroy_cq", rc, 0, result = -1);
	}

	return result;
}

/* try to destroy with QP */
int cq_3(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_pd 		*pd = ERR_PTR(-EINVAL);
	struct ib_cq		*cq = ERR_PTR(-EINVAL);
	struct ib_qp		*qp = ERR_PTR(-EINVAL);
	int			result = -1;
	int			rc;

	TEST_CASE("try to destroy with QP");

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	cq = ib_create_cq(res->device, cq_comp_handler, component_event_handler, NULL, 1, 0);
	CHECK_PTR("ib_create_cq", !IS_ERR(cq), goto cleanup);

	memset(&res->attributes.qp_init_attr, 0, sizeof(res->attributes.qp_init_attr));
	init_qp_cap(res, &res->attributes.qp_init_attr.cap);
	res->attributes.qp_init_attr.recv_cq = cq;
	res->attributes.qp_init_attr.send_cq = cq;
	res->attributes.qp_init_attr.qp_type = VL_range(&res->rand_g, IB_QPT_RC, IB_QPT_UD);
	res->attributes.qp_init_attr.cap.max_send_wr = 1;
	res->attributes.qp_init_attr.cap.max_recv_wr  = 1;
	res->attributes.qp_init_attr.cap.max_send_sge = 1;
	res->attributes.qp_init_attr.cap.max_recv_sge = 1;
	res->attributes.qp_init_attr.event_handler = component_event_handler;

	qp = ib_create_qp(pd, &res->attributes.qp_init_attr);
	CHECK_PTR("ib_create_qp", !IS_ERR(qp), VL_DATA_ERR(("error %ld",PTR_ERR(qp)));  goto cleanup);

	rc = ib_destroy_cq(cq);
	CHECK_VALUE("ib_destroy_cq", rc, -EBUSY, goto cleanup);


	PASSED;

	result = 0;

cleanup:

        if (config->leave_open) {
                VL_MISC_TRACE1(("Resources are OPEN...\n"));
                goto ret;
        }

	if (!IS_ERR(qp)) {
		rc = ib_destroy_qp(qp);
		CHECK_VALUE("ib_destroy_qp", rc, 0, return -1);
	}

	if (!IS_ERR(cq)) {
		rc = ib_destroy_cq(cq);
		CHECK_VALUE("ib_destroy_cq", rc, 0, return -1);
	}

	if (!IS_ERR(pd)) {
		rc = ib_dealloc_pd(pd);
		CHECK_VALUE("ib_dealloc_pd", rc, 0, return -1);
	}

ret:
	return result;
}

/* modify with outstanding */
int cq_4(
	IN	struct resources *res)
{
	struct ib_pd 		*pd = ERR_PTR(-EINVAL);
	struct ib_cq		*cq = ERR_PTR(-EINVAL);
	struct ib_qp		*qp = ERR_PTR(-EINVAL);
	struct ib_mr		*mr = ERR_PTR(-EINVAL);
	dma_addr_t		dma = 0;
	char			*buf = NULL;
	int			num_post;
	int			cqe;
	int			i;
	int			result = -1;
	int			rc;

	TEST_CASE("modify with outstanding to more than original size");

	schedule();

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	cqe = VL_range(&res->rand_g, 1,GET_MAX_CQE(res) / 2); 
    printk("\tcreate CQ with CQE =  %d ",cqe);
	cq = ib_create_cq(res->device, cq_comp_handler, component_event_handler, NULL, cqe, 0);
	CHECK_PTR("ib_create_cq", !IS_ERR(cq), goto cleanup);

	memset(&res->attributes.qp_init_attr, 0, sizeof(res->attributes.qp_init_attr));
	init_qp_cap(res, &res->attributes.qp_init_attr.cap);
	res->attributes.qp_init_attr.recv_cq = cq;
	res->attributes.qp_init_attr.send_cq = cq;
	res->attributes.qp_init_attr.qp_type = IB_QPT_UC;
	res->attributes.qp_init_attr.cap.max_send_wr = 5000;//res->device_attr.max_qp_wr;
	res->attributes.qp_init_attr.cap.max_recv_wr  = 1;
	res->attributes.qp_init_attr.cap.max_send_sge = 1;
	res->attributes.qp_init_attr.cap.max_recv_sge = 1;
	res->attributes.qp_init_attr.event_handler = component_event_handler;
	res->attributes.qp_init_attr.sq_sig_type = 1;

	qp = ib_create_qp(pd, &res->attributes.qp_init_attr);
	CHECK_PTR("ib_create_qp", !IS_ERR(qp), VL_DATA_ERR(("error %ld",PTR_ERR(qp)));  goto cleanup);

	rc = my_modify_qp(&res->rand_g, qp, IB_QPT_UC, VL_range(&res->rand_g, 1, res->device->phys_port_cnt), IB_QPS_RTS, 0);
	CHECK_VALUE("my_modify_qp", rc, 0, goto cleanup);

	buf = kmalloc(1, GFP_DMA);
	CHECK_PTR("kmalloc", buf, goto cleanup);

	dma = ib_dma_map_single(res->device, buf, 1, DMA_BIDIRECTIONAL);
	CHECK_VALUE("ib_dma_map_single", ib_dma_mapping_error(res->device, dma), 0, goto cleanup);

	mr = ib_get_dma_mr(pd, IB_ACCESS_LOCAL_WRITE);
	CHECK_PTR("ib_get_dma_mr", !IS_ERR(mr), goto cleanup);

	num_post = VL_range(&res->rand_g, 1, res->attributes.qp_init_attr.cap.max_send_wr);
	if (num_post > cq->cqe)
		num_post = cq->cqe;

	for (i = 0; i < num_post; ++i) {
		rc = post_send(res, qp, mr, dma, 1);
		CHECK_VALUE("post_send", rc, 0, goto cleanup);
	}

	msleep(5000);

	cqe = VL_range(&res->rand_g, cq->cqe + 1, GET_MAX_CQE(res));
    printk("\tresize CQ to CQE =  %d ",cqe);
	rc= ib_resize_cq(cq, cqe);
	CHECK_VALUE("ib_resize_cq", rc, 0, goto cleanup);

	for (i = 0; i < num_post; ++i) {
		struct ib_wc wc;
		int timeout = 0;

		do {
			rc = ib_poll_cq(cq, 1, &wc);
			timeout++;
		} while (rc == 0 && timeout < 20);
		if (timeout == 20) {
			FAILED;
			VL_DATA_ERR(("ib_poll_cq timeout i = %d", i));
			goto cleanup;
		}
		CHECK_VALUE("ib_poll_cq", rc, 1, goto cleanup);
		CHECK_VALUE("wc status", wc.status, IB_WC_SUCCESS, goto cleanup);
	}


	PASSED;

	TEST_CASE("modify with outstanding to less than original size but more than the number outstanding");

	num_post = VL_range(&res->rand_g, 1, cq->cqe / 2 - 1);
	if (num_post > res->attributes.qp_init_attr.cap.max_send_wr)
		num_post = res->attributes.qp_init_attr.cap.max_send_wr;

	for (i = 0; i < num_post; ++i) {
		rc = post_send(res, qp, mr, dma, 1);
		CHECK_VALUE("post_send", rc, 0, goto cleanup);
	}

	msleep(5000);

	cqe = VL_range(&res->rand_g, num_post + 1, cq->cqe / 2);

	rc= ib_resize_cq(cq, cqe);
	CHECK_VALUE("ib_resize_cq", rc, 0, goto cleanup);

	for (i = 0; i < num_post; ++i) {
		struct ib_wc wc;
		int timeout = 0;

		do {
			rc = ib_poll_cq(cq, 1, &wc);
			timeout++;
		} while (rc == 0 && timeout < 20);
		if (timeout == 20) {
			FAILED;
			VL_DATA_ERR(("ib_poll_cq timeout i = %d", i));
			goto cleanup;
		}
		CHECK_VALUE("ib_poll_cq", rc, 1, goto cleanup);
		CHECK_VALUE("wc status", wc.status, IB_WC_SUCCESS, goto cleanup);
	}


	PASSED;

	TEST_CASE("try modify with outstanding to less than the number outstanding");

	num_post = VL_range(&res->rand_g, cq->cqe / 2 + 1, cq->cqe);
	if (num_post > res->attributes.qp_init_attr.cap.max_send_wr)
		num_post = res->attributes.qp_init_attr.cap.max_send_wr;

	for (i = 0; i < num_post; ++i) {
		rc = post_send(res, qp, mr, dma, 1);
		CHECK_VALUE("post_send", rc, 0, goto cleanup);
	}

	msleep(5000);

	cqe = VL_range(&res->rand_g, 1, num_post / 2); // to avoid next power of 2 problems
    printk("\tresize CQ to CQE =  %d ",cqe);
	rc= ib_resize_cq(cq, cqe);
	CHECK_VALUE("ib_resize_cq", rc, -EINVAL, goto cleanup);

	for (i = 0; i < num_post; ++i) {
		struct ib_wc wc;
		int timeout = 0;

		do {
			rc = ib_poll_cq(cq, 1, &wc);
			timeout++;
		} while (rc == 0 && timeout < 20);
		if (timeout == 20) {
			FAILED;
			VL_DATA_ERR(("ib_poll_cq timeout i = %d", i));
			goto cleanup;
		}
		CHECK_VALUE("ib_poll_cq", rc, 1, goto cleanup);
		CHECK_VALUE("wc status", wc.status, IB_WC_SUCCESS, goto cleanup);
	}


	PASSED;

	result = 0;

cleanup:

	if (!IS_ERR(qp)) {
		rc = ib_destroy_qp(qp);
		CHECK_VALUE("ib_destroy_qp", rc, 0, return -1);
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

	thread_res = result;

	up(&thread_sync);

	return result;
}

static int connect_qp(
	IN	struct resources *res,
	IN	struct ib_qp *qp,
	IN	int port)
{
	struct ib_qp_attr 	attr;
	int			mask;
	int			rc;

	attr.qp_state        	= IB_QPS_INIT;
	attr.pkey_index      	= 0;
	attr.port_num        	= port;
	attr.qp_access_flags 	= 0;

	mask = IB_QP_STATE | IB_QP_PKEY_INDEX | IB_QP_PORT | IB_QP_ACCESS_FLAGS;

	rc = ib_modify_qp(qp, &attr, mask);
	CHECK_VALUE("ib_modify_qp", rc , 0, return -1);

	attr.qp_state		= IB_QPS_RTR;
	attr.path_mtu		= IB_MTU_1024;
	attr.dest_qp_num	= qp->qp_num;
	attr.rq_psn 		= 1;
	attr.max_dest_rd_atomic	= 1;
	attr.min_rnr_timer	= 12;
	attr.ah_attr.ah_flags	= 0;
	attr.ah_attr.dlid	= res->port_attr.lid;
	attr.ah_attr.sl		= 0;
	attr.ah_attr.src_path_bits = 0;
	attr.ah_attr.port_num	= port;

	mask = IB_QP_STATE | IB_QP_AV | IB_QP_PATH_MTU | IB_QP_DEST_QPN | IB_QP_RQ_PSN | IB_QP_MAX_DEST_RD_ATOMIC | IB_QP_MIN_RNR_TIMER;

	rc = ib_modify_qp(qp, &attr, mask);
	CHECK_VALUE("ib_modify_qp", rc , 0, return -1);

	attr.qp_state 	= IB_QPS_RTS;
	attr.timeout 	= 14;
	attr.retry_cnt 	= 7;
	attr.rnr_retry 	= 7;
	attr.sq_psn 	= 1;
	attr.max_rd_atomic  = 1;

	mask = IB_QP_STATE | IB_QP_SQ_PSN | IB_QP_TIMEOUT | IB_QP_RETRY_CNT | IB_QP_RNR_RETRY | IB_QP_MAX_QP_RD_ATOMIC;

	rc = ib_modify_qp(qp, &attr, mask);
	CHECK_VALUE("ib_modify_qp", rc , 0, return -1);

	return 0;
}

static int test_req_notify(
	IN	struct ib_cq *recv_cq,
	IN	struct ib_mr *mr,
	IN	struct ib_qp *qp,
	IN	dma_addr_t dma)
{
	struct ib_sge 		sg;
	struct ib_send_wr 	send_wr;
	struct ib_recv_wr	recv_wr;
	struct ib_send_wr 	*bad_send_wr;
	struct ib_recv_wr 	*bad_recv_wr;
	struct ib_wc 		wc;
	int			comp_count = 0;
	int			timeout = 0;
	int			rc;

	memset(&send_wr, 0, sizeof(send_wr));
	memset(&recv_wr, 0, sizeof(recv_wr));

	sg.addr = dma;
	sg.length = 1;
	sg.lkey = mr->lkey;

	send_wr.next = NULL;
	send_wr.num_sge = 1;
	send_wr.opcode = IB_WR_SEND;
	send_wr.send_flags = 0;
	send_wr.sg_list = &sg;
	send_wr.wr_id = 1;

	recv_wr.next = NULL;
	recv_wr.num_sge = 1;
	recv_wr.sg_list = &sg;
	recv_wr.wr_id = 2;

	rc = ib_post_recv(qp, &recv_wr, &bad_recv_wr);
	CHECK_VALUE("ib_post_recv", rc, 0, return -1);

	rc = ib_post_recv(qp, &recv_wr, &bad_recv_wr);
	CHECK_VALUE("ib_post_recv", rc, 0, return -1);

	rc = ib_post_recv(qp, &recv_wr, &bad_recv_wr);
	CHECK_VALUE("ib_post_recv", rc, 0, return -1);

	/* the SR won't be posted with flag IB_SEND_SOLICITED so there shouldn't be a completion event */
	rc = ib_req_notify_cq(recv_cq, IB_CQ_SOLICITED);
	CHECK_VALUE("ib_req_notify_cq", rc, 0, return -1);

	rc = ib_post_send(qp, &send_wr, &bad_send_wr);
	CHECK_VALUE("ib_post_send", rc, 0, return -1);

	timeout = 0;
	do {
		rc = ib_poll_cq(recv_cq, 1, &wc);
		if (rc == 0) {
			timeout++;
			msleep(100);
		}
	} while (rc == 0 && timeout < 20);
	if (timeout == 20) {
		FAILED;
		VL_DATA_ERR(("ib_poll_cq timeout 1"));
		return -1;
	}
	CHECK_VALUE("ib_poll_cq", rc, 1, return -1);
	CHECK_VALUE("wc status", wc.status, IB_WC_SUCCESS, return -1);

	/* There should be a completion aevent, although the SR wasn't posted with flag IB_SEND_SOLICITED */
	rc = ib_req_notify_cq(recv_cq, IB_CQ_NEXT_COMP);
	CHECK_VALUE("ib_req_notify_cq", rc, 0, return -1);

	rc = ib_post_send(qp, &send_wr, &bad_send_wr);
	CHECK_VALUE("ib_post_send", rc, 0, return -1);

	timeout = 0;
        do {
		rc = ib_poll_cq(recv_cq, 1, &wc);
		if (rc == 0) {
			timeout++;
			msleep(100);
		}
	} while (rc == 0 && timeout < 50);
	if (timeout == 20) {
		FAILED;
		VL_DATA_ERR(("ib_poll_cq timeout 2"));
		return -1;
	}
	CHECK_VALUE("ib_poll_cq", rc, 1, return -1);
	CHECK_VALUE("wc status", wc.status, IB_WC_SUCCESS, return -1);

	send_wr.send_flags = IB_SEND_SOLICITED;

	 /* There should be a completion aevent, because the SR will posted with flag IB_SEND_SOLICITED */
	rc = ib_req_notify_cq(recv_cq, IB_CQ_SOLICITED);
	CHECK_VALUE("ib_req_notify_cq", rc, 0, return -1);

	rc = ib_post_send(qp, &send_wr, &bad_send_wr);
	CHECK_VALUE("ib_post_send", rc, 0, return -1);

	/* There should have been 2 completion events, so the completion event handler - cq_comp_handler
	should have been called twice and therefor num_comp should have been incremented twice */
        do {
		spin_lock(&comp_sync);
		comp_count = num_comp;
		spin_unlock(&comp_sync);
		timeout++;
		msleep(100);
	} while (comp_count < 2 && timeout < 20);
	if (timeout == 20) {
		FAILED;
		VL_DATA_ERR(("CB timeout"));
		return -1;
	}
        CHECK_VALUE("num_comp", comp_count, 2, return -1);

	PASSED;
#if 0
	TEST_CASE("ib_req_ncomp_notif");

	num_comp = 0;

	num_post = VL_range(&res->rand_g, 1, res->attributes.qp_init_attr.cap.max_send_wr - 3); // we allready posted 3 wr to the SQ

	rc = ib_req_ncomp_notif(send_cq, num_post);
	CHECK_VALUE("ib_req_ncomp_notif", rc, 0, return -1);

	send_wr.send_flags = IB_SEND_SIGNALED;

	for (i = 0; i < num_post; ++i) {
		rc = ib_post_recv(qp, &recv_wr, &bad_recv_wr);
		CHECK_VALUE("ib_post_recv", rc, 0, return -1);
	}

	for (i = 0; i < num_post; ++i) {
		rc = ib_post_send(qp, &send_wr, &bad_send_wr);
		CHECK_VALUE("ib_post_send", rc, 0, return -1);
	}

	timeout = 0;
	do {
		spin_lock(&comp_sync);
		comp_count = num_comp;
		spin_unlock(&comp_sync);
		timeout++;
		msleep(10);
	} while (comp_count < 1 && timeout < 200);
	if (timeout == 200) {
		FAILED;
		VL_DATA_ERR(("CB timeout"));
		return -1;
	}
        CHECK_VALUE("num_comp", comp_count, 1, return -1);

	num_post = VL_range(&res->rand_g, 0, num_post);

	rc = ib_req_ncomp_notif(send_cq, num_post);
	CHECK_VALUE("ib_req_ncomp_notif", rc, 0, return -1);

	timeout = 0;
	do {
		spin_lock(&comp_sync);
		comp_count = num_comp;
		spin_unlock(&comp_sync);
		timeout++;
		msleep(10);
	} while (comp_count < 2 && timeout < 200);
	if (timeout == 200) {
		FAILED;
		VL_DATA_ERR(("CB timeout"));
		return -1;
	}
        CHECK_VALUE("num_comp", comp_count, 2, return -1);

	PASSED;
#endif

	return 0;
}

/* ib_req_notify_cq ib_req_ncomp_notif */
int cq_5(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_pd 		*pd = ERR_PTR(-EINVAL);
	struct ib_cq		*send_cq = ERR_PTR(-EINVAL);
	struct ib_cq		*recv_cq = ERR_PTR(-EINVAL);
	struct ib_qp		*qp = ERR_PTR(-EINVAL);
	struct ib_mr		*mr = ERR_PTR(-EINVAL);
	dma_addr_t		dma = 0;
	char			*buf = NULL;
	int			port = VL_range(&res->rand_g, 1, res->device->phys_port_cnt);
	int			cqe;
	int			result = -1;
	int			rc;

	TEST_CASE("ib_req_notify_cq");
	schedule();

	spin_lock_init(&comp_sync);
	num_comp = 0;

	rc = ib_query_port(res->device, port, &res->port_attr);
	CHECK_VALUE("ib_query_port", rc , 0, goto cleanup);

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

    //cqe = res->device_attr.max_cqe;
    cqe = GET_MAX_CQE(res);

    send_cq = ib_create_cq(res->device, cq_comp_handler, component_event_handler, NULL, cqe, 0);
	CHECK_PTR("ib_create_cq", !IS_ERR(send_cq), goto cleanup);

	recv_cq = ib_create_cq(res->device, cq_comp_handler, component_event_handler, NULL, cqe, 0);
	CHECK_PTR("ib_create_cq", !IS_ERR(recv_cq), goto cleanup);

	memset(&res->attributes.qp_init_attr, 0, sizeof(res->attributes.qp_init_attr));
	init_qp_cap(res, &res->attributes.qp_init_attr.cap);
	res->attributes.qp_init_attr.recv_cq = recv_cq;
	res->attributes.qp_init_attr.send_cq = send_cq;
	res->attributes.qp_init_attr.qp_type = IB_QPT_RC;
	res->attributes.qp_init_attr.cap.max_send_wr = 100;
	res->attributes.qp_init_attr.cap.max_recv_wr  = 100;
	res->attributes.qp_init_attr.cap.max_send_sge = 1;
	res->attributes.qp_init_attr.cap.max_recv_sge = 1;
	res->attributes.qp_init_attr.event_handler = component_event_handler;
	res->attributes.qp_init_attr.sq_sig_type = IB_SIGNAL_REQ_WR;

	qp = ib_create_qp(pd, &res->attributes.qp_init_attr);
	CHECK_PTR("ib_create_qp", !IS_ERR(qp), VL_DATA_ERR(("error %ld",PTR_ERR(qp)));  goto cleanup);

	rc = connect_qp(res, qp, port);
	CHECK_VALUE("connect_qp", rc , 0, goto cleanup);

	buf = kmalloc(1, GFP_DMA);
	CHECK_PTR("kmalloc", buf, goto cleanup);

	dma = ib_dma_map_single(res->device, buf, 1, DMA_BIDIRECTIONAL);
	CHECK_VALUE("ib_dma_map_single", ib_dma_mapping_error(res->device, dma), 0, goto cleanup);

	mr = ib_get_dma_mr(pd, IB_ACCESS_LOCAL_WRITE);
	CHECK_PTR("ib_get_dma_mr", !IS_ERR(mr), goto cleanup);

	rc = test_req_notify(recv_cq, mr, qp, dma);
	CHECK_VALUE("test_req_notify", rc , 0, goto cleanup);

	result = 0;

cleanup:

        if (config->leave_open) {
                VL_MISC_TRACE1(("Resources are OPEN...\n"));
                goto ret;
        }

	if (!IS_ERR(qp)) {
		rc = ib_destroy_qp(qp);
		CHECK_VALUE("ib_destroy_qp", rc, 0, return -1);
	}

	if (!IS_ERR(send_cq)) {
		rc = ib_destroy_cq(send_cq);
		CHECK_VALUE("ib_destroy_cq", rc, 0, return -1);
	}

	if (!IS_ERR(recv_cq)) {
		rc = ib_destroy_cq(recv_cq);
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

ret:
	return result;
}

/* Check reserved LKEY */
int cq_6(
        IN	struct config_t *config,
        IN	struct resources *res,
        IN	struct ib_client *test_client)
{
	struct ib_pd 		*pd = ERR_PTR(-EINVAL);
	struct ib_cq		*cq = ERR_PTR(-EINVAL);
	struct ib_qp		*qp = ERR_PTR(-EINVAL);
	struct ib_mr		*mr = ERR_PTR(-EINVAL);
	dma_addr_t		dma = 0;
	char			*buf = NULL;
	int			num_post;
	int			cqe;
	int			i;
	int			result = -1;
	int			rc;

	TEST_CASE("\nCheck Reserved LKEY support for ConnectX\n");

	schedule();

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	cqe = VL_range(&res->rand_g, 1, GET_MAX_CQE(res) / 2); 

	cq = ib_create_cq(res->device, cq_comp_handler, component_event_handler, NULL, cqe, 0);
	CHECK_PTR("ib_create_cq", !IS_ERR(cq), goto cleanup);

	memset(&res->attributes.qp_init_attr, 0, sizeof(res->attributes.qp_init_attr));
	init_qp_cap(res, &res->attributes.qp_init_attr.cap);
	res->attributes.qp_init_attr.recv_cq = cq;
	res->attributes.qp_init_attr.send_cq = cq;
	res->attributes.qp_init_attr.qp_type = IB_QPT_UC;
	res->attributes.qp_init_attr.cap.max_send_wr = 5000;//res->device_attr.max_qp_wr;
	res->attributes.qp_init_attr.cap.max_recv_wr  = 1;
	res->attributes.qp_init_attr.cap.max_send_sge = 1;
	res->attributes.qp_init_attr.cap.max_recv_sge = 1;
	res->attributes.qp_init_attr.event_handler = component_event_handler;
	res->attributes.qp_init_attr.sq_sig_type = 1;

	qp = ib_create_qp(pd, &res->attributes.qp_init_attr);
	CHECK_PTR("ib_create_qp", !IS_ERR(qp), VL_DATA_ERR(("error %ld",PTR_ERR(qp)));  goto cleanup);

	rc = my_modify_qp(&res->rand_g, qp, IB_QPT_UC, VL_range(&res->rand_g, 1, res->device->phys_port_cnt), IB_QPS_RTS, 0);
	CHECK_VALUE("my_modify_qp", rc, 0, goto cleanup);

	buf = kmalloc(1, GFP_DMA);
	CHECK_PTR("kmalloc", buf, goto cleanup);

	dma = ib_dma_map_single(res->device, buf, 1, DMA_BIDIRECTIONAL);
	CHECK_VALUE("ib_dma_map_single", ib_dma_mapping_error(res->device, dma), 0, goto cleanup);

	if (!res->dma_local_lkey) {
		mr = ib_get_dma_mr(pd, IB_ACCESS_LOCAL_WRITE);
		CHECK_PTR("ib_get_dma_mr", !IS_ERR(mr), goto cleanup);
	} else {
		VL_MISC_TRACE1(("Use RESERVED LKEY\n"));
		mr = NULL;
	}

	VL_MISC_TRACE1(("PD %p CQ %p QP %p QP_NUM 0x%x\n", pd, cq, qp, qp->qp_num));

	num_post = VL_range(&res->rand_g, 1, res->attributes.qp_init_attr.cap.max_send_wr);
	if (num_post > cq->cqe)
		num_post = cq->cqe;

	for (i = 0; i < num_post; ++i) {
		rc = post_send(res, qp, mr, dma, 1);
		CHECK_VALUE("post_send", rc, 0, goto cleanup);
	}

	msleep(5000);

	for (i = 0; i < num_post; ++i) {
		struct ib_wc wc;
		int timeout = 0;

		do {
			rc = ib_poll_cq(cq, 1, &wc);
			timeout++;
		} while (rc == 0 && timeout < 20);
		if (timeout == 20) {
			FAILED;
			VL_DATA_ERR(("ib_poll_cq timeout i = %d", i));
			goto cleanup;
		}
		CHECK_VALUE("ib_poll_cq", rc, 1, goto cleanup);
		CHECK_VALUE("wc status", wc.status, IB_WC_SUCCESS, goto cleanup);
	}


	PASSED;

	result = 0;

cleanup:
        if (config->leave_open) {
                VL_MISC_TRACE1(("Resources are OPEN...\n"));
                goto ret;
        }

	if (!IS_ERR(qp)) {
		rc = ib_destroy_qp(qp);
		CHECK_VALUE("ib_destroy_qp", rc, 0, return -1);
	}

	if (!IS_ERR(cq)) {
		rc = ib_destroy_cq(cq);
		CHECK_VALUE("ib_destroy_cq", rc, 0, return -1);
	}

	if (!res->dma_local_lkey) {
		if (!IS_ERR(mr)) {
			rc = ib_dereg_mr(mr);
			CHECK_VALUE("ib_dereg_mr", rc, 0, return -1);
		}
	}

	if (!IS_ERR(pd)) {
		rc = ib_dealloc_pd(pd);
		CHECK_VALUE("ib_dealloc_pd", rc, 0, return -1);
	}

	if (buf) {
		ib_dma_unmap_single(res->device, dma, 1, DMA_BIDIRECTIONAL);
		kfree(buf);
	}

ret:
	thread_res = result;

	up(&thread_sync);

	return result;
}

int test_cq(
	IN	struct config_t* config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	init_MUTEX(&thread_sync);

	RUN_TEST(1, cq_1);
	RUN_TEST(2, cq_2);
	RUN_TEST(3, cq_3);

	if (config->test_num == 4) {
		int rc;

		thread_res = -1;

		rc = down_interruptible(&thread_sync);
		rc = kernel_thread((void*)cq_4, (void*)res, 0);
	
		/* when the kernel thread will finish the work, it will release the mutex */
		rc = down_interruptible(&thread_sync);
		up(&thread_sync);

		CHECK_VALUE("cq_4", thread_res, 0, return thread_res);
	}
	RUN_TEST(5, cq_5);
	RUN_TEST(6, cq_6);

	return 0;
}
