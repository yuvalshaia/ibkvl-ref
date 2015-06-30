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
 * $Id: test_hca.c 7876 2009-07-29 14:47:25Z ronniz $ 
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


struct ib_event	test_event;
static struct semaphore event_sync;
static int event_res = -1;

// TODO:
// 1. set port


void component_event_handler(
	IN	struct ib_event *event, 
	IN	void *context)
{

	return;
}

void my_event_handler(
	IN	struct ib_event_handler *handler,
	IN	struct ib_event *event)
{

	VL_HCA_TRACE1(("enter my_event_handler"));

	CHECK_VALUE("device", (unsigned long)event->device, (unsigned long)test_event.device, goto err);
	CHECK_VALUE("event", (unsigned long)event->event, (unsigned long)test_event.event, goto err);

	switch (event->event) {
	case IB_EVENT_CQ_ERR:
		CHECK_VALUE("cq", (unsigned long)event->element.cq, (unsigned long)test_event.element.cq, goto err);
		break;

	case IB_EVENT_QP_FATAL:
	case IB_EVENT_QP_REQ_ERR:
	case IB_EVENT_QP_ACCESS_ERR:
	case IB_EVENT_COMM_EST:
	case IB_EVENT_SQ_DRAINED:
	case IB_EVENT_PATH_MIG:
	case IB_EVENT_PATH_MIG_ERR:
	case IB_EVENT_QP_LAST_WQE_REACHED:
		CHECK_VALUE("qp", (unsigned long)event->element.qp, (unsigned long)test_event.element.qp, goto err);
		break;
	
	case IB_EVENT_PORT_ACTIVE:
	case IB_EVENT_PORT_ERR:
	case IB_EVENT_LID_CHANGE:
	case IB_EVENT_PKEY_CHANGE:
	case IB_EVENT_SM_CHANGE:
#ifdef _IB_EVENT_GID_CHANGE_EXIST
	case IB_EVENT_GID_CHANGE:
#endif
	case IB_EVENT_CLIENT_REREGISTER:
		CHECK_VALUE("port_num", (unsigned long)event->element.port_num, (unsigned long)test_event.element.port_num,
goto err);
		break;
	
	case IB_EVENT_SRQ_ERR:
	case IB_EVENT_SRQ_LIMIT_REACHED:
		CHECK_VALUE("srq", (unsigned long)event->element.srq, (unsigned long)test_event.element.srq, goto err);
		break;

	case IB_EVENT_DEVICE_FATAL:
		break;
	}

	event_res = 0;

err:
	up(&event_sync);
}


/* ib_set_client_data ib_get_client_data */
int hca_1(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	void *context = NULL;

	TEST_CASE("ib_set_client_data ib_get_client_data");

	context = (void *)(unsigned long)VL_random(&res->rand_g, 0xFFFFFFFF);

	ib_set_client_data(res->device, test_client, context);

	CHECK_VALUE("ib_get_client_data", (unsigned long)ib_get_client_data(res->device, test_client), (unsigned long)context, return -1);

	PASSED;

	return 0;
}

/* ib_register_event_handler ib_unregister_event_handler */
int hca_2(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_event_handler *event_handler = NULL;
	struct ib_event_handler *handler_array = NULL;
	int			num_handlers;
	int			i;
	int			result = -1;
	int 			rc;

	TEST_CASE("ib_register_event_handler ib_unregister_event_handler");

	event_handler = kmalloc(sizeof(*event_handler), GFP_KERNEL);
	CHECK_PTR("kmalloc", event_handler, goto cleanup);

	event_handler->device = res->device;
	event_handler->handler = my_event_handler;

	rc = ib_register_event_handler(event_handler);
	CHECK_VALUE("ib_register_event_handler", rc, 0, goto cleanup);

	rc = ib_unregister_event_handler(event_handler);
	CHECK_VALUE("ib_register_event_handler", rc, 0, goto cleanup);

	PASSED;

	TEST_CASE("register many events");

	num_handlers = VL_range(&res->rand_g, 2, 100);

	handler_array = kmalloc(num_handlers * sizeof(struct ib_event_handler), GFP_KERNEL);
	CHECK_PTR("kmalloc", handler_array, goto cleanup);

	for (i = 0; i < num_handlers; ++i) {
		handler_array[i].device = res->device;
		handler_array[i].handler = my_event_handler;
	}

	for (i = 0; i < num_handlers; ++i) {
		rc = ib_register_event_handler(&handler_array[i]);
		CHECK_VALUE("ib_register_event_handler", rc, 0, goto cleanup);
	}

	for (i = 0; i < num_handlers; ++i) {
		rc = ib_unregister_event_handler(&handler_array[i]);
		CHECK_VALUE("ib_register_event_handler", rc, 0, goto cleanup);
	}

	PASSED;


	result = 0;

cleanup:

	if (event_handler)
		kfree(event_handler);

	if (handler_array)
		kfree(handler_array);

	return result;
}

/* ib_dispatch_event */
int hca_3(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_event_handler *event_handler = NULL;
	struct ib_pd		*pd = ERR_PTR(-EINVAL);
	struct ib_cq		*cq = ERR_PTR(-EINVAL);
	struct ib_qp		*qp = ERR_PTR(-EINVAL);
	struct ib_srq		*srq = ERR_PTR(-EINVAL);
	int			result = -1;
	int			i;
	int 			rc;

	TEST_CASE("ib_dispatch_event");

	memset(&test_event, 0, sizeof(test_event));
	
	//TODO : Saeed Fix kernel 3.
	init_MUTEX(&event_sync);
	//sema_init(&event_sync,1);

	event_handler = kmalloc(sizeof(*event_handler), GFP_KERNEL);
	CHECK_PTR("kmalloc", event_handler, goto cleanup);

	event_handler->device = res->device;
	event_handler->handler = my_event_handler;

	rc = ib_register_event_handler(event_handler);
	CHECK_VALUE("ib_register_event_handler", rc, 0, goto cleanup);

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	cq = ib_create_cq(res->device, cq_comp_handler, component_event_handler, NULL, 1, 0);
	CHECK_PTR("ib_create_cq", !IS_ERR(cq), goto cleanup);

	memset(&res->attributes.srq_init_attr, 0, sizeof(res->attributes.srq_init_attr));
	res->attributes.srq_init_attr.event_handler = component_event_handler;
	init_srq_cap(config, res, &res->attributes.srq_init_attr);

	srq = ib_create_srq(pd, &res->attributes.srq_init_attr);
	CHECK_PTR("ib_create_srq", !IS_ERR(srq), goto cleanup);

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

	test_event.device = res->device;
        i = IB_EVENT_CLIENT_REREGISTER;
		rc = down_interruptible(&event_sync);
		CHECK_VALUE("down_interruptible", rc, 0, goto cleanup);
		test_event.element.port_num = 1;
		test_event.event = i;
		ib_dispatch_event(&test_event);
		rc = down_interruptible(&event_sync);
		CHECK_VALUE("down_interruptible", rc, 0, goto cleanup);
		up(&event_sync);
		CHECK_VALUE("event_res", event_res, 0, goto cleanup);
		event_res = -1;
	
	i = IB_EVENT_LID_CHANGE;
                rc = down_interruptible(&event_sync);
                CHECK_VALUE("down_interruptible", rc, 0, goto cleanup);
                test_event.element.port_num = 1;
                test_event.event = i;
                ib_dispatch_event(&test_event);
                rc = down_interruptible(&event_sync);
                CHECK_VALUE("down_interruptible", rc, 0, goto cleanup);
                up(&event_sync);
                CHECK_VALUE("event_res", event_res, 0, goto cleanup);
                event_res = -1;
   
 

	PASSED;

	result = 0;

cleanup:

	if (event_handler) {
		rc = ib_unregister_event_handler(event_handler);
		CHECK_VALUE("ib_register_event_handler", rc, 0, result = -1);

		kfree(event_handler);
	}

	if (!IS_ERR(qp)) {
		rc = ib_destroy_qp(qp);
		CHECK_VALUE("ib_destroy_qp", rc, 0, result = -1);
	}

	if (!IS_ERR(srq)) {
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

/* ib_query_port */
int hca_4(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	uint8_t i;
	int rc;

	TEST_CASE("ib_query_port");

	for (i = 1; i <= res->device->phys_port_cnt; ++i) {
		rc = ib_query_port(res->device, i, &res->port_attr);
		CHECK_VALUE("ib_query_port", rc, 0, return -1);

	} 

	PASSED;

	return 0;
}

/* ib_query_gid */
int hca_5(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	union ib_gid	gid;
	uint8_t 	i;
	int		j;
	int 		rc;

	TEST_CASE("ib_query_gid");

	for (i = 1; i <= res->device->phys_port_cnt; ++i) {
		rc = ib_query_port(res->device, i, &res->port_attr);
		CHECK_VALUE("ib_query_port", rc, 0, return -1);

		for (j = 0; j < res->port_attr.gid_tbl_len; ++j) {
			rc = ib_query_gid(res->device, i, j, &gid);
			CHECK_VALUE("ib_query_gid", rc, 0, VL_HCA_ERR(("xxxxx j = %d tbl len %d",j, res->port_attr.gid_tbl_len)); return -1);
		}
	} 

	PASSED;

	return 0;
}

/* ib_query_pkey */
int hca_6(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	u16		pkey;
	uint8_t 	i;
	u16		j;
	int 		rc;

	TEST_CASE("ib_query_pkey");

	for (i = 1; i <= res->device->phys_port_cnt; ++i) {
		rc = ib_query_port(res->device, i, &res->port_attr);
		CHECK_VALUE("ib_query_port", rc, 0, return -1);

		for (j = 0; j < res->port_attr.pkey_tbl_len; ++j) {
			rc = ib_query_pkey(res->device, i, j, &pkey);
			CHECK_VALUE("ib_query_pkey", rc, 0, VL_HCA_ERR(("xxxxx j = %d tbl len %d",j, res->port_attr.gid_tbl_len)); return -1);
		}
	} 

	PASSED;

	return 0;
}

/* ib_modify_device */
int hca_7(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_device_modify	modify_attr;
	struct ib_device_modify	temp_attr;
	int			i;
	int			result = -1;
	int 			rc;

	TEST_CASE("ib_modify_device");

	memcpy(temp_attr.node_desc, res->device->node_desc, 64);
	temp_attr.sys_image_guid = res->device_attr.sys_image_guid;

	for (i = 0; i < 64; ++i)
		modify_attr.node_desc[i] = VL_random(&res->rand_g, 0xFF);
	modify_attr.sys_image_guid = VL_random64(&res->rand_g, 0xFFFFFFFFFFFFFFFFULL);

	rc = ib_modify_device(res->device, IB_DEVICE_MODIFY_NODE_DESC, &modify_attr);
	CHECK_VALUE("ib_modify_device", rc, 0, goto cleanup);

	for (i = 0; i < 64; ++i)
		CHECK_VALUE("node desc", res->device->node_desc[i], modify_attr.node_desc[i], goto cleanup);

/*	NOT SUPPORTED YET

	rc = ib_modify_device(res->device, IB_DEVICE_MODIFY_SYS_IMAGE_GUID, &modify_attr);
	CHECK_VALUE("ib_modify_device", rc, 0, goto cleanup);

	rc = ib_query_device(res->device, &res->device_attr);
	CHECK_VALUE("ib_query_device", rc, 0, goto cleanup);

	CHECK_VALUE("sys_image_guid", res->device_attr.sys_image_guid, modify_attr.sys_image_guid, goto cleanup);*/

	PASSED;

	result = 0;
cleanup:
	rc = ib_modify_device(res->device, /*IB_DEVICE_MODIFY_SYS_IMAGE_GUID | */IB_DEVICE_MODIFY_NODE_DESC, &temp_attr);
	CHECK_VALUE("ib_modify_device", rc, 0, result = -1);

	return result;
}

/* ib_modify_port */
int hca_8(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_port_attr 	*attr_array = NULL;
	struct ib_port_modify	attr;
	uint8_t			port;
	uint8_t 		i;
	int			attr_modified = 0;
	int			result = -1;
	int 			rc;

	TEST_CASE("modify port's cap mask");

	/* Create the array with an extra cell so each port's data can be located in an array cell matching it's 
	port index */
	attr_array = kmalloc((res->device->phys_port_cnt + 1)* sizeof(*attr_array), GFP_KERNEL);
	CHECK_PTR("kmalloc", attr_array, goto cleanup);

	for (port = 1; port <= res->device->phys_port_cnt; ++port) {
		rc = ib_query_port(res->device, port, &attr_array[port]);
		CHECK_VALUE("ib_query_port", rc, 0, goto cleanup);
	}

	for (port = 1; port <= res->device->phys_port_cnt; ++port) {
		memset(&attr, 0, sizeof(attr));

		for (i = 1; i <= 25; ++i) {
			attr.set_port_cap_mask = 1 << i;

			rc = ib_modify_port(res->device, port, 0, &attr);
			CHECK_VALUE("ib_modify_port", rc, 0, goto cleanup);

			attr_modified = 1;

			rc = ib_query_port(res->device, port, &res->port_attr);
			CHECK_VALUE("ib_query_port", rc, 0, goto cleanup);

			if (!(res->port_attr.port_cap_flags & (1 << i))) {
				VL_HCA_ERR(("error in ib_modify_port of attr %d", i));
				goto cleanup;
			}

			attr.set_port_cap_mask = 0;
			attr.clr_port_cap_mask = 1 << i;

			rc = ib_modify_port(res->device, port, 0, &attr);
			CHECK_VALUE("ib_modify_port", rc, 0, goto cleanup);

			rc = ib_query_port(res->device, port, &res->port_attr);
			CHECK_VALUE("ib_query_port", rc, 0, goto cleanup);

			if ((res->port_attr.port_cap_flags << (31 - i)) >> 31) {
				VL_HCA_ERR(("error in ib_modify_port of attr %d", i));
				goto cleanup;
			}
		}

		attr.set_port_cap_mask = attr_array[port].port_cap_flags;
		rc = ib_modify_port(res->device, port, 0, &attr);
		CHECK_VALUE("ib_modify_port", rc, 0, goto cleanup);
	}

	PASSED;

	TEST_CASE("reset pkey counter");

	for (port = 1; port <= res->device->phys_port_cnt; ++port) {
		memset(&attr, 0, sizeof(attr));

                rc = ib_modify_port(res->device, port, IB_PORT_RESET_QKEY_CNTR, &attr);
		CHECK_VALUE("ib_modify_port", rc, 0, goto cleanup);

		rc = ib_query_port(res->device, port, &res->port_attr);
		CHECK_VALUE("ib_query_port", rc, 0, goto cleanup);

		CHECK_VALUE("qkey_viol_cntr", res->port_attr.qkey_viol_cntr, 0, goto cleanup);
	}

	PASSED;


	result = 0;
cleanup:
	if (attr_array) {
		if (attr_modified) {
			for (port = 1; port <= res->device->phys_port_cnt; ++port) {
				attr.set_port_cap_mask = attr_array[port].port_cap_flags;
				attr.clr_port_cap_mask = ~attr_array[port].port_cap_flags;

				rc = ib_modify_port(res->device, port, 0, &attr);
				CHECK_VALUE("ib_modify_port", rc, 0, result = -1);
			}
		}

		kfree(attr_array);
	}

	return result;
}

int test_hca(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	RUN_TEST(1, hca_1);
	RUN_TEST(2, hca_2);
	RUN_TEST(3, hca_3);
	RUN_TEST(4, hca_4);
	RUN_TEST(5, hca_5);
	RUN_TEST(6, hca_6);
	RUN_TEST(7, hca_7);
	RUN_TEST(8, hca_8);

	return 0;
}
