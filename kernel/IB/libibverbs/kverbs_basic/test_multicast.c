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
 * $Id: test_multicast.c 7876 2009-07-29 14:47:25Z ronniz $ 
 * 
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h>       /* for register char device */
#include <linux/sched.h>
#include <linux/list.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
        #include <asm/semaphore.h>      /* for semaphore */
#else
        #include <linux/semaphore.h>
#endif

#include <vl_ib_verbs.h>

#include <vl.h>
#include "my_types.h"
#include "func_headers.h"

static struct semaphore thread_sync;
static int thread_res = -1;

void set_start_gid(
        union ib_gid *gid_p)
{
        memset(gid_p, 0, sizeof(union ib_gid));

        gid_p->raw[0] = 255;
        gid_p->raw[1] = 1;
        gid_p->raw[9] = 2;
        gid_p->raw[10] = 201;
        gid_p->raw[11] = 0;
        gid_p->raw[12] = 1;
        gid_p->raw[13] = 0;
        gid_p->raw[14] = 208;
        gid_p->raw[15] = 0;
}

void increment_gid(
        union ib_gid *gid_p)
{
        int i = 16;


        do {
                i --;
                gid_p->raw[i] ++;

        } while (gid_p->raw[i] == 0);
}

/* ib_attach_mcast ib_detach_mcast */
int multicast_1(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_pd 		*pd = ERR_PTR(-EINVAL);
	struct ib_cq		*cq = ERR_PTR(-EINVAL);
	struct ib_qp		*qp = ERR_PTR(-EINVAL);
	union ib_gid 		gid;
	u16	 		lid;
	int			i;
	int			result = -1;
	int			rc;

	TEST_CASE("ib_attach_mcast ib_detach_mcast");

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	cq = ib_create_cq(res->device, cq_comp_handler, component_event_handler, NULL, 1, 0);
	CHECK_PTR("ib_create_cq", !IS_ERR(cq), goto cleanup);

	memset(&res->attributes.qp_init_attr, 0, sizeof(res->attributes.qp_init_attr));
	init_qp_cap(res, &res->attributes.qp_init_attr.cap);
	res->attributes.qp_init_attr.recv_cq = cq;
	res->attributes.qp_init_attr.send_cq = cq;
	res->attributes.qp_init_attr.qp_type = IB_QPT_UD;
	res->attributes.qp_init_attr.event_handler = component_event_handler;

	qp = ib_create_qp(pd, &res->attributes.qp_init_attr);
	CHECK_PTR("ib_create_qp", !IS_ERR(qp), goto cleanup);

	gid.raw[0] = 0xff;
	for (i = 1; i < 16; ++i)
		gid.raw[i] = VL_random(&res->rand_g, 0xFF);

	lid = VL_range(&res->rand_g, 0xC000, 0xFFFE);

	rc = ib_attach_mcast(qp, &gid, lid);
	CHECK_VALUE("ib_attach_mcast", rc, 0, goto cleanup);

	rc = ib_detach_mcast(qp, &gid, lid);
	CHECK_VALUE("ib_detach_mcast", rc, 0, goto cleanup);

	PASSED;

	result = 0;

cleanup:

	if (qp != NULL && !IS_ERR(qp)) {
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

/* attach qp to max mcast groups */
int multicast_2(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_pd 		*pd = ERR_PTR(-EINVAL);
	struct ib_cq		*cq = ERR_PTR(-EINVAL);
	struct ib_qp		*qp = ERR_PTR(-EINVAL);
	union ib_gid 		gid;
	union ib_gid		gid_backup;
	u16	 		lid;
	int			i;
	int			last_mcast_idx = 0;
	int			result = -1;
	int			rc;

	TEST_CASE("attach qp to max mcast groups");

	lid = VL_range(&res->rand_g, 0xC000, 0xFFFE);

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	cq = ib_create_cq(res->device, cq_comp_handler, component_event_handler, NULL, 1, 0);
	CHECK_PTR("ib_create_cq", !IS_ERR(cq), goto cleanup);

	memset(&res->attributes.qp_init_attr, 0, sizeof(res->attributes.qp_init_attr));
	init_qp_cap(res, &res->attributes.qp_init_attr.cap);
	res->attributes.qp_init_attr.recv_cq = cq;
	res->attributes.qp_init_attr.send_cq = cq;
	res->attributes.qp_init_attr.qp_type = IB_QPT_UD;
	res->attributes.qp_init_attr.event_handler = component_event_handler;

	qp = ib_create_qp(pd, &res->attributes.qp_init_attr);
	CHECK_PTR("ib_create_qp", !IS_ERR(qp), goto cleanup);

	set_start_gid(&gid);

	for (i = 0; i < res->device_attr.max_mcast_grp - 1; ++i) {
		rc = ib_attach_mcast(qp, &gid, lid);
		CHECK_VALUE("ib_attach_mcast", rc, 0, VL_DATA_ERR(("i = %d max_mcast_grp = %d", i, res->device_attr.max_mcast_grp)); goto cleanup);

		increment_gid(&gid);
		last_mcast_idx = i;
	}

	PASSED;

	result = 0;

cleanup:

	set_start_gid(&gid_backup);
	for (i = 0; i <= last_mcast_idx; ++i) {
		rc = ib_detach_mcast(qp, &gid_backup, lid);
		CHECK_VALUE("ib_detach_mcast", rc, 0, VL_DATA_ERR(("i = %d max_mcast_grp = %d", i, res->device_attr.max_mcast_grp)); result = -1;);

		increment_gid(&gid_backup);
	}

	if (qp != NULL && !IS_ERR(qp)) {
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

/* attach max qps to mcast group */
int multicast_3(
	IN	struct resources *res)
{
	struct ib_pd 		*pd = ERR_PTR(-EINVAL);
	struct ib_cq		*cq = ERR_PTR(-EINVAL);
	struct list_head	head;
	struct qp_list_node     *curr = NULL;
	struct qp_list_node	*tmp = NULL;
	union ib_gid 		gid;
	u16	 		lid;
	int			i;
	int			result = -1;
	int			rc;

	TEST_CASE("attach max qps to mcast group");
	schedule();

	INIT_LIST_HEAD(&head);

	lid = VL_range(&res->rand_g, 0xC000, 0xFFFE);

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	cq = ib_create_cq(res->device, cq_comp_handler, component_event_handler, NULL, 1, 0);
	CHECK_PTR("ib_create_cq", !IS_ERR(cq), goto cleanup);

	memset(&res->attributes.qp_init_attr, 0, sizeof(res->attributes.qp_init_attr));
	res->attributes.qp_init_attr.cap.max_recv_sge = 1;
	res->attributes.qp_init_attr.cap.max_recv_wr = 1;
	res->attributes.qp_init_attr.cap.max_send_sge = 1;
	res->attributes.qp_init_attr.cap.max_send_wr = 1;
	res->attributes.qp_init_attr.recv_cq = cq;
	res->attributes.qp_init_attr.send_cq = cq;
	res->attributes.qp_init_attr.qp_type = IB_QPT_UD;
	res->attributes.qp_init_attr.event_handler = component_event_handler;

	for (i = 0; i < res->device_attr.max_mcast_qp_attach; i++) {
		curr = kmalloc(sizeof(*curr), GFP_KERNEL);
		CHECK_PTR("kmalloc", curr, goto cleanup);

		curr->qp = ERR_PTR(-EINVAL);

		curr->qp = ib_create_qp(pd, &res->attributes.qp_init_attr);
		CHECK_PTR("ib_create_qp", !IS_ERR(curr->qp), kfree(curr); VL_DATA_ERR(("error %ld i %d max %d", 
			PTR_ERR(curr->qp), i, res->device_attr.max_total_mcast_qp_attach));goto cleanup);

		list_add_tail(&curr->list, &head);

		curr = NULL;
	}

	gid.raw[0] = 0xff;
	for (i = 1; i < 16; ++i)
		gid.raw[i] = VL_random(&res->rand_g, 0xFF);

	i = 0;
	list_for_each_entry(curr, &head, list) {
		rc = ib_attach_mcast(curr->qp, &gid, lid);
		CHECK_VALUE("ib_attach_mcast", rc, 0, VL_DATA_ERR(("i = %d, max_mcast_qp_attach %d", i, res->device_attr.max_mcast_qp_attach)); goto cleanup);
		i++;
	}

        PASSED;

	result = 0;

cleanup:

	schedule();
	list_for_each_entry(curr, &head, list) {
		rc = ib_detach_mcast(curr->qp, &gid, lid);
		CHECK_VALUE("ib_detach_mcast", rc, 0, result = -1);
	}

	list_for_each_entry_safe(curr, tmp, &head, list) {
		rc = ib_destroy_qp(curr->qp);
		CHECK_VALUE("ib_destroy_qp", rc, 0, result = -1);

	        kfree(curr);
	}

	if (!IS_ERR(cq)) {
		rc = ib_destroy_cq(cq);
		CHECK_VALUE("ib_destroy_cq", rc, 0, result = -1);
	}

	if (!IS_ERR(pd)) {
		rc = ib_dealloc_pd(pd);
		CHECK_VALUE("ib_dealloc_pd", rc, 0, result = -1);
	}

	thread_res = result;

	up(&thread_sync);

	return result;
}

int test_multicast(
	IN	struct config_t* config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	init_MUTEX(&thread_sync);

	RUN_TEST(1, multicast_1);
	RUN_TEST(2, multicast_2);

	if (config->test_num == 3) {
		int rc;

		thread_res = -1;
		if(down_interruptible(&thread_sync))
		{
			VL_DATA_ERR(("test aborted"));
			return -1;
		}
		rc = kernel_thread((void*)multicast_3, (void*)res, 0);
	
		/* when the kernel thread will finish the work, it will release the mutex */
		if(down_interruptible(&thread_sync))
		{
			VL_DATA_ERR(("test aborted"));
			return -1;
		}
		up(&thread_sync);

		CHECK_VALUE("multicast_3", thread_res, 0, return thread_res);
	}

	return 0;
}
