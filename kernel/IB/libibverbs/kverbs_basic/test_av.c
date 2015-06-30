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
 * $Id: test_av.c 5370 2008-04-24 11:15:42Z ronniz $ 
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

#define MAX_WATCH_DOG_VALUE 1000

uint8_t get_static_rate(
	IN	int valid_value,
	IN	struct VL_random_t *rand_gen)
{
    uint8_t static_rate;
    int watchdog = 0;

    if (valid_value) {
        watchdog = 0;
        do {
            static_rate = VL_random(rand_gen, 12);
            watchdog ++;
            if (watchdog >= MAX_WATCH_DOG_VALUE) {
                static_rate = 0;
                break;
            }
        } while ((static_rate != 0) && (static_rate != 1) && (static_rate != 2) && (static_rate != 3) && (static_rate != 11));

    } else {
        do {
            static_rate = VL_range(rand_gen, 1, 63);
            watchdog ++;
            if (watchdog >= MAX_WATCH_DOG_VALUE) {
                static_rate = 5;
                break;
            }
        } while ((static_rate == 0) || (static_rate == 1) || (static_rate == 2) || (static_rate == 3) || (static_rate == 11));
    }

    return static_rate;
}

void create_random_av_attr(
	IN	struct resources *res,
	OUT	struct ib_ah_attr *attr)
{
	int i;

	attr->ah_flags = VL_random(&res->rand_g, 2);
	attr->port_num = VL_range(&res->rand_g, 1, res->device->phys_port_cnt);
	attr->sl = VL_range(&res->rand_g, 1, 15);
	attr->dlid = VL_range(&res->rand_g, 1, 0xffff);
	attr->src_path_bits = VL_random(&res->rand_g, 128);
	attr->static_rate = 3;//get_static_rate(1, &res->rand_g);

	attr->grh.dgid.raw[0] = 0xff;
	for (i = 1; i < 16; ++i)
		attr->grh.dgid.raw[i] = VL_random(&res->rand_g, 0xff);

	attr->grh.flow_label = VL_random(&res->rand_g, 0xFFFFF);//0x100000);
	attr->grh.hop_limit = VL_range(&res->rand_g, 1, 0xff);
	attr->grh.sgid_index = VL_random(&res->rand_g, res->port_attr.gid_tbl_len);
	attr->grh.traffic_class = VL_range(&res->rand_g, 1, 0xff);
}

/* ib_create_ah ib_destroy_ah */
int av_1(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_pd 		*pd = ERR_PTR(-EINVAL);
	struct ib_ah		*av = ERR_PTR(-EINVAL);
	struct ib_ah_attr	av_attr;
	u8			use_grh;
	u8			port;
	u8			sl;
	int			i;
	int			result = -1;
	int			rc;

	TEST_CASE("ib_create_ah ib_destroy_ah");

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	for (use_grh = 0; use_grh < 2; ++use_grh) {
		for (port = 1; port <= res->device->phys_port_cnt; ++port) {
			for (sl = 0; sl < 16; ++sl) {
				memset(&av_attr, 0, sizeof(av_attr));

				av_attr.ah_flags = use_grh;
				av_attr.port_num = port;
				av_attr.sl = sl;
				av_attr.dlid = VL_range(&res->rand_g, 1, 0xffff);
				av_attr.src_path_bits = VL_random(&res->rand_g, 128);
				av_attr.static_rate = get_static_rate(1, &res->rand_g);

				av_attr.grh.dgid.raw[0] = 0xff;
				for (i = 1; i < 16; ++i)
					av_attr.grh.dgid.raw[i] = VL_random(&res->rand_g, 0xff);
				av_attr.grh.flow_label = VL_random(&res->rand_g, 0x100000);
				av_attr.grh.hop_limit = VL_range(&res->rand_g, 1, 0xff);
				av_attr.grh.sgid_index = VL_random(&res->rand_g, res->port_attr.gid_tbl_len);
				av_attr.grh.traffic_class = VL_range(&res->rand_g, 1, 0xff);

				av = ib_create_ah(pd, &av_attr);
				CHECK_PTR("ib_create_ah", !IS_ERR(av), goto cleanup);

				rc = ib_destroy_ah(av);
				CHECK_VALUE("ib_destroy_ah", rc, 0, goto cleanup);
				av = ERR_PTR(-EINVAL);
			}
		}
	}
	

	PASSED;

	result = 0;

cleanup:

	if (!IS_ERR(pd)) {
		rc = ib_dealloc_pd(pd);
		CHECK_VALUE("ib_dealloc_pd", rc, 0, result = -1);
	}
	

	return result;
}

/* ib_query_ah */
int av_2(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_pd 		*pd = ERR_PTR(-EINVAL);
	struct ib_ah		*av = ERR_PTR(-EINVAL);
	struct ib_ah_attr	av_attr;
	struct ib_ah_attr	av_query;
	u8			use_grh;
	u8			port;
	u8			sl;
	int			i;
	int			result = -1;
	int			rc;

	TEST_CASE("ib_query_ah");

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	for (use_grh = 0; use_grh < 2; ++use_grh) {
		for (port = 1; port <= res->device->phys_port_cnt; ++port) {
			for (sl = 0; sl < 16; ++sl) {
				memset(&av_attr, 0, sizeof(av_attr));

				av_attr.ah_flags = use_grh;
				av_attr.port_num = port;
				av_attr.sl = sl;
				av_attr.dlid = VL_range(&res->rand_g, 1, 0xffff);
				av_attr.src_path_bits = VL_random(&res->rand_g, 128);
				av_attr.static_rate = 3;//get_static_rate(1, &res->rand_g);

				av_attr.grh.dgid.raw[0] = 0xff;
				for (i = 1; i < 16; ++i)
					av_attr.grh.dgid.raw[i] = VL_random(&res->rand_g, 0xff);
				av_attr.grh.flow_label = VL_random(&res->rand_g, 0x100000);
				av_attr.grh.hop_limit = VL_range(&res->rand_g, 1, 0xff);
				av_attr.grh.sgid_index = VL_random(&res->rand_g, res->port_attr.gid_tbl_len);
				av_attr.grh.traffic_class = VL_range(&res->rand_g, 1, 0xff);

				av = ib_create_ah(pd, &av_attr);
				CHECK_PTR("ib_create_ah", !IS_ERR(av), av = NULL; goto cleanup);

				rc = ib_query_ah(av, &av_query);
				CHECK_VALUE("ib_query_ah", rc, 0, goto cleanup);

				CHECK_VALUE("ah_flags", av_query.ah_flags, av_attr.ah_flags, goto cleanup);
				CHECK_VALUE("port_num", av_query.port_num, av_attr.port_num, goto cleanup);
				CHECK_VALUE("sl", av_query.sl, av_attr.sl, goto cleanup);
				CHECK_VALUE("dlid", av_query.dlid, av_attr.dlid, goto cleanup);
				CHECK_VALUE("src_path_bits", av_query.src_path_bits, av_attr.src_path_bits, goto cleanup);
				CHECK_VALUE("static_rate", av_query.static_rate, av_attr.static_rate, goto cleanup);

				if (av_query.ah_flags) {
					CHECK_VALUE("grh.flow_label", av_query.grh.flow_label, av_attr.grh.flow_label, goto cleanup);
					CHECK_VALUE("grh.hop_limit", av_query.grh.hop_limit, av_attr.grh.hop_limit, goto cleanup);
					CHECK_VALUE("grh.sgid_index", av_query.grh.sgid_index, av_attr.grh.sgid_index, goto cleanup);
					CHECK_VALUE("grh.traffic_class", av_query.grh.traffic_class, av_attr.grh.traffic_class, goto cleanup);

					for (i = 0; i < 16; ++i) {
						CHECK_VALUE("grh.dgid", av_query.grh.dgid.raw[i], av_attr.grh.dgid.raw[i], 
							    VL_DATA_ERR(("error in i %d", i));goto cleanup);
					}
				}
                                

				rc = ib_destroy_ah(av);
				CHECK_VALUE("ib_destroy_ah", rc, 0, goto cleanup);
				av = ERR_PTR(-EINVAL);
			}
		}
	}
	

	PASSED;

	result = 0;

cleanup:
	if (!IS_ERR(av)) {
		rc = ib_destroy_ah(av);
		CHECK_VALUE("ib_destroy_ah", rc, 0, result = -1);
	}

	if (!IS_ERR(pd)) {
		rc = ib_dealloc_pd(pd);
		CHECK_VALUE("ib_dealloc_pd", rc, 0, result = -1);
	}
	

	return result;
}

/* ib_modify_ah */
int av_3(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_pd 		*pd = ERR_PTR(-EINVAL);
	struct ib_ah		*av = ERR_PTR(-EINVAL);
	struct ib_ah_attr	av_attr;
	struct ib_ah_attr	av_query;
	u8			use_grh;
	u8			port;
	u8			sl;
	int			i;
	int			result = -1;
	int			rc;

	TEST_CASE("ib_query_ah");

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	for (use_grh = 0; use_grh < 2; ++use_grh) {
		for (port = 1; port <= res->device->phys_port_cnt; ++port) {
			for (sl = 0; sl < 16; ++sl) {
				memset(&av_attr, 0, sizeof(av_attr));

				create_random_av_attr(res, &av_attr);

				av = ib_create_ah(pd, &av_attr);
				CHECK_PTR("ib_create_ah", !IS_ERR(av), goto cleanup);

				av_attr.ah_flags = use_grh;
				av_attr.port_num = port;
				av_attr.sl = sl;
				av_attr.dlid = VL_range(&res->rand_g, 1, 0xffff);
				av_attr.src_path_bits = VL_random(&res->rand_g, 128);
				av_attr.static_rate = 3;//get_static_rate(1, &res->rand_g);

				av_attr.grh.dgid.raw[0] = 0xff;
				for (i = 1; i < 16; ++i)
					av_attr.grh.dgid.raw[i] = VL_random(&res->rand_g, 0xff);
				av_attr.grh.flow_label = VL_random(&res->rand_g, 0x100000);
				av_attr.grh.hop_limit = VL_range(&res->rand_g, 1, 0xff);
				av_attr.grh.sgid_index = VL_random(&res->rand_g, res->port_attr.gid_tbl_len);
				av_attr.grh.traffic_class = VL_range(&res->rand_g, 1, 0xff);

				rc = ib_modify_ah(av, &av_attr);
				CHECK_VALUE("ib_modify_ah", rc, 0, goto cleanup);

				rc = ib_query_ah(av, &av_query);
				CHECK_VALUE("ib_destroy_ah", rc, 0, goto cleanup);

				CHECK_VALUE("ah_flags", av_query.ah_flags, av_attr.ah_flags, goto cleanup);
				CHECK_VALUE("port_num", av_query.port_num, av_attr.port_num, goto cleanup);
				CHECK_VALUE("sl", av_query.sl, av_attr.sl, goto cleanup);
				CHECK_VALUE("dlid", av_query.dlid, av_attr.dlid, goto cleanup);
				CHECK_VALUE("src_path_bits", av_query.src_path_bits, av_attr.src_path_bits, goto cleanup);
				CHECK_VALUE("static_rate", av_query.static_rate, av_attr.static_rate, goto cleanup);

				if (av_query.ah_flags) {
					CHECK_VALUE("grh.flow_label", av_query.grh.flow_label, av_attr.grh.flow_label, goto cleanup);
					CHECK_VALUE("grh.hop_limit", av_query.grh.hop_limit, av_attr.grh.hop_limit, goto cleanup);
					CHECK_VALUE("grh.sgid_index", av_query.grh.sgid_index, av_attr.grh.sgid_index, goto cleanup);
					CHECK_VALUE("grh.traffic_class", av_query.grh.traffic_class, av_attr.grh.traffic_class, goto cleanup);

					for (i = 0; i < 16; ++i) {
						CHECK_VALUE("grh.dgid", av_query.grh.dgid.raw[i], av_attr.grh.dgid.raw[i], 
							    VL_DATA_ERR(("error in i %d", i));goto cleanup);
					}
				}
                                

				rc = ib_destroy_ah(av);
				CHECK_VALUE("ib_destroy_ah", rc, 0, goto cleanup);
				av = ERR_PTR(-EINVAL);
			}
		}
	}
	

	PASSED;

	result = 0;

cleanup:
	if (!IS_ERR(av)) {
		rc = ib_destroy_ah(av);
		CHECK_VALUE("ib_destroy_ah", rc, 0, result = -1);
	}

	if (!IS_ERR(pd)) {
		rc = ib_dealloc_pd(pd);
		CHECK_VALUE("ib_dealloc_pd", rc, 0, result = -1);
	}
	

	return result;
}

/* ib_create_ah_from_wc */
int av_4(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_pd 		*pd = ERR_PTR(-EINVAL);
	struct ib_ah		*av = ERR_PTR(-EINVAL);
	u8			use_grh;
	u8			port;
	u8			sl;
	struct ib_wc		wc;
	struct ib_grh		grh;
	int			i;
	int			result = -1;
	int			rc;

	TEST_CASE("ib_create_ah_from_wc");

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	for (use_grh = 0; use_grh < 1/*2*/; ++use_grh) {
		for (port = 1; port <= res->device->phys_port_cnt; ++port) {
			for (sl = 0; sl < 16; ++sl) {
				u32 flow_label;
				u8 traffic_class;

				wc.dlid_path_bits = VL_random(&res->rand_g, 128);
				wc.port_num = port;
				wc.sl = sl;
				wc.slid = VL_range(&res->rand_g, 1, 0xffff);

				wc.opcode = IB_WC_RECV;
				wc.wc_flags = use_grh ? IB_WC_GRH : 0;
                             
				grh.sgid.raw[0] = 0xff;
				for (i = 1; i < 16; ++i)
					grh.sgid.raw[i] = VL_random(&res->rand_g, 0xff);

				grh.hop_limit = VL_range(&res->rand_g, 1, 0xff);
				flow_label = VL_random(&res->rand_g, 0x100000);
				traffic_class = VL_range(&res->rand_g, 1, 0xff);

				grh.version_tclass_flow = (traffic_class << 20) | flow_label;
				grh.version_tclass_flow = cpu_to_be32(grh.version_tclass_flow);

                                av = ib_create_ah_from_wc(pd, &wc, &grh, port);
				CHECK_PTR("ib_create_ah_from_wc", !IS_ERR(av), VL_DATA_ERR(("error %ld",PTR_ERR(av))); goto cleanup);

				rc = ib_destroy_ah(av);
				CHECK_VALUE("ib_destroy_ah", rc, 0, goto cleanup);
				av = ERR_PTR(-EINVAL);
			}
		}
	}
	

	PASSED;

	result = 0;

cleanup:

	if (!IS_ERR(pd)) {
		rc = ib_dealloc_pd(pd);
		CHECK_VALUE("ib_dealloc_pd", rc, 0, result = -1);
	}
	

	return result;
}

int test_av(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	RUN_TEST(1, av_1);
	RUN_TEST(2, av_2);
/* modify not supported yet??	RUN_TEST(3, av_3); */
	RUN_TEST(4, av_4);

	return 0;
}
