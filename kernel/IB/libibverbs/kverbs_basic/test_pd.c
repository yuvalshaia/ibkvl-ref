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
 * $Id: test_pd.c 7877 2009-07-29 14:49:45Z ronniz $ 
 * 
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h>       /* for register char device */
#include <vl_ib_verbs.h>

#include <vl.h>
#include "my_types.h"
#include "func_headers.h"

/* ib_alloc_pd ib_dealloc_pd */
int pd_1(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_pd 	*pd = ERR_PTR(-EINVAL);
	int		rc;

	TEST_CASE("ib_alloc_pd ib_dealloc_pd");

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), return -1);

	rc = ib_dealloc_pd(pd);
	CHECK_VALUE("ib_dealloc_pd", rc, 0, return -1);

	PASSED;

	return 0;
}

/* try to destroy with AV */
int pd_2(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_pd 		*pd = ERR_PTR(-EINVAL);
	struct ib_ah		*av = ERR_PTR(-EINVAL);
	struct ib_ah_attr 	attr;
	int			result = -1;
	int			rc;

	TEST_CASE("try to destroy with AV");

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	create_random_av_attr(res, &attr);

	av = ib_create_ah(pd, &attr);
	CHECK_PTR("ib_create_ah", !IS_ERR(av), goto cleanup);

	rc = ib_dealloc_pd(pd);
	CHECK_VALUE("ib_dealloc_pd", rc, -EBUSY, goto cleanup);


	PASSED;

	result = 0;

cleanup:

	if (!IS_ERR(av)) {
		rc = ib_destroy_ah(av);
		CHECK_VALUE("ib_destroy_ah", rc, 0, return -1);
	}

	if (!IS_ERR(pd)) {
		rc = ib_dealloc_pd(pd);
		CHECK_VALUE("ib_dealloc_pd", rc, 0, return -1);
	}

	return result;
}

/* try to destroy with QP */
int pd_3(
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

	rc = ib_dealloc_pd(pd);
	CHECK_VALUE("ib_dealloc_pd", rc, -EBUSY, goto cleanup);


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

	if (!IS_ERR(pd)) {
		rc = ib_dealloc_pd(pd);
		CHECK_VALUE("ib_dealloc_pd", rc, 0, return -1);
	}

	return result;
}

/* try to destroy with MR */
int pd_4(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_pd 		*pd = ERR_PTR(-EINVAL);
	struct ib_mr		*mr = ERR_PTR(-EINVAL);
	dma_addr_t		dma = 0;
	char			*buf = NULL;
	int			result = -1;
	int			rc;

	TEST_CASE("try to destroy with MR");

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	buf = kmalloc(1, GFP_DMA);
	CHECK_PTR("kmalloc", buf, goto cleanup);

	dma = ib_dma_map_single(res->device, buf, 1, DMA_BIDIRECTIONAL);
	CHECK_VALUE("ib_dma_map_single", ib_dma_mapping_error(res->device, dma), 0, goto cleanup);

	mr = ib_get_dma_mr(pd, IB_ACCESS_LOCAL_WRITE);
	CHECK_PTR("ib_get_dma_mr", !IS_ERR(mr), goto cleanup);

	rc = ib_dealloc_pd(pd);
	CHECK_VALUE("ib_dealloc_pd", rc, -EBUSY, goto cleanup);


	PASSED;

	result = 0;

cleanup:

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


	return result;
}

/* try to destroy with SRQ */
int pd_5(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_pd 		*pd = ERR_PTR(-EINVAL);
	struct ib_srq		*srq = ERR_PTR(-EINVAL);
	int			result = -1;
	int			rc;

	TEST_CASE("try to destroy with SRQ");

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	memset(&res->attributes.srq_init_attr, 0, sizeof(res->attributes.srq_init_attr));
	res->attributes.srq_init_attr.event_handler = component_event_handler;
	init_srq_cap(config, res, &res->attributes.srq_init_attr);

	srq = ib_create_srq(pd, &res->attributes.srq_init_attr);
	CHECK_PTR("ib_create_srq", !IS_ERR(srq), goto cleanup);

	rc = ib_dealloc_pd(pd);
	CHECK_VALUE("ib_dealloc_pd", rc, -EBUSY, goto cleanup);


	PASSED;

	result = 0;

cleanup:

	if (!IS_ERR(srq)) {
		rc = ib_destroy_srq(srq);
		CHECK_VALUE("ib_destroy_srq", rc, 0, return -1);
	}

	if (!IS_ERR(pd)) {
		rc = ib_dealloc_pd(pd);
		CHECK_VALUE("ib_dealloc_pd", rc, 0, return -1);
	}

	return result;
}


int test_pd(
	IN	struct config_t* config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	RUN_TEST(1, pd_1);
	RUN_TEST(2, pd_2);
	RUN_TEST(3, pd_3);
	RUN_TEST(4, pd_4);
	RUN_TEST(5, pd_5);

	return 0;
}
