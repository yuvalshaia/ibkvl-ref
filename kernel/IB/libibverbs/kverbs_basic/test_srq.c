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
 * $Id: test_srq.c 7876 2009-07-29 14:47:25Z ronniz $ 
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

#include <vl_ib_verbs.h>

#include <vl.h>
#include "my_types.h"
#include "func_headers.h"

// TODO:
// 1. srq init cap
// 2. srq modify WR

static struct semaphore event_sync;
static int event_res = -1;
void* srq_context;

void init_srq_cap(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN OUT	struct ib_srq_init_attr *cap)
{
	cap->attr.max_sge = VL_range(&res->rand_g, 1, res->device_attr.max_srq_sge);
	cap->attr.max_wr = VL_range(&res->rand_g, 1, 100); // res->device_attr.max_srq_wr); - todo
}

void srq_event_handler(
	IN	struct ib_event *event, 
	IN	void *context)
{
	if (event->event != IB_EVENT_SRQ_LIMIT_REACHED) {
		VL_HCA_ERR(("srq_event_handler - wrong event reached %d", event->event));
		event_res = -1;
	} else {
		event_res = 0;
		CHECK_VALUE("srq context", (unsigned long)context, (unsigned long)srq_context, event_res = -1);
	}

	up(&event_sync);

	return;
}

/* ib_create_srq ib_destroy_srq */
int srq_1(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_pd 		*pd = ERR_PTR(-EINVAL);
	struct ib_srq		*srq = ERR_PTR(-EINVAL);
	int			result = -1;
	int			rc;

	TEST_CASE("ib_create_srq ib_destroy_srq");

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	res->attributes.srq_init_attr.event_handler = srq_event_handler;
	init_srq_cap(config, res, &res->attributes.srq_init_attr);

	srq = ib_create_srq(pd, &res->attributes.srq_init_attr);
	CHECK_PTR("ib_create_srq", !IS_ERR(srq), goto cleanup);

	rc = ib_destroy_srq(srq);
	CHECK_VALUE("ib_destroy_srq", rc, 0, goto cleanup);

	PASSED;

	result = 0;

cleanup:

	if (!IS_ERR(pd)) {
		rc = ib_dealloc_pd(pd);
		CHECK_VALUE("ib_dealloc_pd", rc, 0, return -1);
	}

	return result;
}

/* ib_modify_srq ib_query_srq */
int srq_2(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_pd 		*pd = ERR_PTR(-EINVAL);
	struct ib_srq		*srq = ERR_PTR(-EINVAL);
	struct ib_recv_wr	wr;
	struct ib_recv_wr 	*bad_wr;
	struct ib_sge		sge;
	int			num_post;
	struct ib_srq_attr	srq_modify_attr;
	struct ib_srq_attr	srq_query_attr;
	int			i;
	int			result = -1;
	int			rc;

	TEST_CASE("ib_query_srq - query init attr");

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	memset(&res->attributes.srq_init_attr.attr, 0, sizeof(res->attributes.srq_init_attr.attr));
	res->attributes.srq_init_attr.event_handler = srq_event_handler;
	init_srq_cap(config, res, &res->attributes.srq_init_attr);

	srq = ib_create_srq(pd, &res->attributes.srq_init_attr);
	CHECK_PTR("ib_create_srq", !IS_ERR(srq), goto cleanup);

	rc = ib_query_srq(srq, &srq_query_attr);
	CHECK_VALUE("ib_query_srq", rc, 0, goto cleanup);

	CHECK_VALUE("max_sge", srq_query_attr.max_sge, res->attributes.srq_init_attr.attr.max_sge, goto cleanup);
	CHECK_VALUE("max_wr", srq_query_attr.max_wr, res->attributes.srq_init_attr.attr.max_wr, goto cleanup);
	CHECK_VALUE("srq_limit", srq_query_attr.srq_limit, res->attributes.srq_init_attr.attr.srq_limit, goto cleanup);

	PASSED;

	TEST_CASE("ib_modify_srq ib_query_srq - query modified attr");

	sge.addr = VL_random64(&res->rand_g, 0xFFFFFFFFFFFFFFFFULL);
	sge.length = VL_random(&res->rand_g, 0xFFFFFFFF);
	sge.lkey = VL_random(&res->rand_g, 0xFFFFFFFF);

	memset(&wr, 0, sizeof(wr));
	wr.num_sge = 1;
	wr.sg_list = &sge;

	num_post = VL_range(&res->rand_g, 1, res->attributes.srq_init_attr.attr.max_wr);
	for (i = 0; i < num_post; ++i) {
		rc = ib_post_srq_recv(srq, &wr, &bad_wr);
		CHECK_VALUE("ib_post_srq_recv", rc, 0, VL_DATA_ERR(("i %d, num post %d, max_wr %d", i, num_post, srq_query_attr.max_wr)); goto cleanup);
	}

	srq_modify_attr.max_wr = VL_range(&res->rand_g, num_post + 1, 101); // res->device_attr.max_srq_wr); - todo
	srq_modify_attr.srq_limit = VL_random(&res->rand_g, num_post - 1);

	rc = ib_modify_srq(srq, &srq_modify_attr, /*IB_SRQ_MAX_WR | */IB_SRQ_LIMIT); // TODO SRQ modify WR not SUPPORTED YET !!!!!
	CHECK_VALUE("ib_modify_srq", rc, 0, goto cleanup);

	rc = ib_query_srq(srq, &srq_query_attr);
	CHECK_VALUE("ib_query_srq", rc, 0, goto cleanup);

	CHECK_VALUE("max_sge", srq_query_attr.max_sge, res->attributes.srq_init_attr.attr.max_sge, goto cleanup);
	/*CHECK_VALUE("max_wr", srq_query_attr.max_wr, srq_modify_attr.max_wr, goto cleanup);*/
	CHECK_VALUE("srq_limit", srq_query_attr.srq_limit, srq_modify_attr.srq_limit, goto cleanup);

        
	PASSED;

	result = 0;

cleanup:

	if (!IS_ERR(srq)) {
		rc = ib_destroy_srq(srq);
		CHECK_VALUE("ib_destroy_srq", rc, 0, result = -1);
	}

	if (!IS_ERR(pd)) {
		rc = ib_dealloc_pd(pd);
		CHECK_VALUE("ib_dealloc_pd", rc, 0, result = -1);
	}

	return result;
}

/* try destroy with QP */
int srq_3(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_pd 		*pd = ERR_PTR(-EINVAL);
	struct ib_srq		*srq = ERR_PTR(-EINVAL);
	struct ib_cq		*cq = ERR_PTR(-EINVAL);
	struct ib_qp		*qp = ERR_PTR(-EINVAL);
	int			result = -1;
	int			rc;

	TEST_CASE("try destroy with QP");

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	res->attributes.srq_init_attr.event_handler = srq_event_handler;
	init_srq_cap(config, res, &res->attributes.srq_init_attr);

	srq = ib_create_srq(pd, &res->attributes.srq_init_attr);
	CHECK_PTR("ib_create_srq", !IS_ERR(srq), goto cleanup);

	cq = ib_create_cq(res->device, cq_comp_handler, component_event_handler, NULL, 1, 0);
	CHECK_PTR("ib_create_cq", !IS_ERR(cq), goto cleanup);

	memset(&res->attributes.qp_init_attr, 0, sizeof(res->attributes.qp_init_attr));
	init_qp_cap(res, &res->attributes.qp_init_attr.cap);
	res->attributes.qp_init_attr.recv_cq = cq;
	res->attributes.qp_init_attr.send_cq = cq;
	res->attributes.qp_init_attr.srq = srq;
	res->attributes.qp_init_attr.qp_type = VL_range(&res->rand_g, IB_QPT_RC, IB_QPT_UD);
	res->attributes.qp_init_attr.event_handler = component_event_handler;

	qp = ib_create_qp(pd, &res->attributes.qp_init_attr);
	CHECK_PTR("ib_create_qp", !IS_ERR(qp), VL_DATA_ERR(("error %ld",PTR_ERR(qp)));  goto cleanup);

	rc = ib_destroy_srq(srq);
	CHECK_VALUE("ib_destroy_srq", rc, -EBUSY, goto cleanup);

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

	if (!IS_ERR(srq)) {
		rc = ib_destroy_srq(srq);
		CHECK_VALUE("ib_destroy_srq", rc, 0, result = -1);
	}

	if (!IS_ERR(pd)) {
		rc = ib_dealloc_pd(pd);
		CHECK_VALUE("ib_dealloc_pd", rc, 0, return -1);
	}

	return result;
}

/* limit event */
int srq_4(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_pd 		*pd = ERR_PTR(-EINVAL);
	struct ib_srq		*srq = ERR_PTR(-EINVAL);
	struct ib_srq_attr	srq_modify_attr;
	int			result = -1;
	int			rc;

	TEST_CASE("limit event");

	init_MUTEX(&event_sync);

	if(down_interruptible(&event_sync))
	{
		VL_DATA_ERR(("test aborted"));
		result = -1;
		goto cleanup;
	}

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	memset(&res->attributes.srq_init_attr.attr, 0, sizeof(res->attributes.srq_init_attr.attr));
	res->attributes.srq_init_attr.event_handler = srq_event_handler;
	srq_context = res->attributes.srq_init_attr.srq_context = (void *)(unsigned long)VL_random(&res->rand_g, 0xFFFFFFFF);
	init_srq_cap(config, res, &res->attributes.srq_init_attr);

	srq = ib_create_srq(pd, &res->attributes.srq_init_attr);
	CHECK_PTR("ib_create_srq", !IS_ERR(srq), goto cleanup);

	srq_modify_attr.srq_limit = VL_range(&res->rand_g, 1, res->attributes.srq_init_attr.attr.max_wr);

	rc = ib_modify_srq(srq, &srq_modify_attr, IB_SRQ_LIMIT);
	CHECK_VALUE("ib_modify_srq", rc, 0, goto cleanup);

	if(down_interruptible(&event_sync))
	{
		VL_DATA_ERR(("test aborted"));
		result = -1;
		goto cleanup;
	}
	up(&event_sync);

	CHECK_VALUE("event_res", event_res, 0, goto cleanup);
        
	PASSED;

	result = 0;

cleanup:

	if (!IS_ERR(srq)) {
		rc = ib_destroy_srq(srq);
		CHECK_VALUE("ib_destroy_srq", rc, 0, result = -1);
	}

	if (!IS_ERR(pd)) {
		rc = ib_dealloc_pd(pd);
		CHECK_VALUE("ib_dealloc_pd", rc, 0, result = -1);
	}

	return result;
}

int test_srq(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	RUN_TEST(1, srq_1);
	RUN_TEST(2, srq_2);
	RUN_TEST(3, srq_3);
	RUN_TEST(4, srq_4);

	return 0;
}
