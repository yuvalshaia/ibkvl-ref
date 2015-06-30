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
 * $Id: test_qp.c 10709 2010-12-14 15:23:19Z saeedm $ 
 * 
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h>       /* for register char device */
#include <linux/delay.h>

#include <vl_ib_verbs.h>

#include "my_types.h"
#include "func_headers.h"

enum test_type {
	REQUIRED_ATTR,       	// 0
	OPTIONAL_ATTR		// 1
};

struct test_vector_t {
	enum ib_qp_state from;
	enum ib_qp_state to;
	int required_attr;
	int optional_attr;
};

struct test_vector_t test_vector[] = {
	{ 
		IB_QPS_RESET, 
		IB_QPS_INIT, 
		IB_QP_STATE | IB_QP_PKEY_INDEX | IB_QP_PORT | IB_QP_QKEY | IB_QP_ACCESS_FLAGS, 
		0 
	},
	{ 
		IB_QPS_INIT, 
		IB_QPS_INIT, 
		IB_QP_STATE, 
		IB_QP_ACCESS_FLAGS | IB_QP_PKEY_INDEX | IB_QP_PORT | IB_QP_QKEY 
	},
	{ 
		IB_QPS_INIT, 
		IB_QPS_RTR, 
		IB_QP_STATE | IB_QP_AV | IB_QP_DEST_QPN | IB_QP_RQ_PSN | IB_QP_MAX_DEST_RD_ATOMIC | IB_QP_MIN_RNR_TIMER |
		IB_QP_PATH_MTU,
		IB_QP_ALT_PATH | IB_QP_ACCESS_FLAGS | IB_QP_PKEY_INDEX /*| IB_QP_CAP*/
	},
	{
		IB_QPS_RTR,
		IB_QPS_RTS,
		IB_QP_STATE | IB_QP_TIMEOUT | IB_QP_RETRY_CNT | IB_QP_RNR_RETRY | IB_QP_SQ_PSN | IB_QP_MAX_QP_RD_ATOMIC,
		IB_QP_ACCESS_FLAGS | IB_QP_QKEY | IB_QP_ALT_PATH | IB_QP_PATH_MIG_STATE /*| IB_QP_CAP*/ | IB_QP_CUR_STATE | 
		IB_QP_MIN_RNR_TIMER
	},
	{
		IB_QPS_RTS,
		IB_QPS_RTS,
		IB_QP_STATE,
		IB_QP_ACCESS_FLAGS | IB_QP_QKEY | IB_QP_ALT_PATH | IB_QP_PATH_MIG_STATE /*| IB_QP_CAP*/ | IB_QP_CUR_STATE |
		IB_QP_MIN_RNR_TIMER
	},
/*	{
		IB_QPS_SQE,
		IB_QPS_RTS,
		IB_QP_STATE,
		IB_QP_ACCESS_FLAGS | IB_QP_QKEY | IB_QP_CAP | IB_QP_CUR_STATE | IB_QP_MIN_RNR_TIMER
	},*/
	{
		IB_QPS_RTS,
		IB_QPS_SQD,
		IB_QP_STATE,
		IB_QP_EN_SQD_ASYNC_NOTIFY
	},
	{
		IB_QPS_SQD,
		IB_QPS_SQD,
		IB_QP_STATE,
		IB_QP_ACCESS_FLAGS | IB_QP_AV | IB_QP_ALT_PATH | IB_QP_PATH_MIG_STATE | IB_QP_MAX_QP_RD_ATOMIC | IB_QP_MAX_DEST_RD_ATOMIC |
		IB_QP_QKEY | IB_QP_PKEY_INDEX | IB_QP_TIMEOUT | IB_QP_RETRY_CNT | IB_QP_RNR_RETRY /*| IB_QP_CAP*/ | IB_QP_PORT |
		IB_QP_MIN_RNR_TIMER
	},
	{
		IB_QPS_SQD,
		IB_QPS_RTS,
		IB_QP_STATE,
		IB_QP_ACCESS_FLAGS | IB_QP_QKEY | IB_QP_ALT_PATH | IB_QP_PATH_MIG_STATE /*| IB_QP_CAP */| IB_QP_CUR_STATE | 
		IB_QP_MIN_RNR_TIMER
	},
	{
		IB_QPS_INIT,
		IB_QPS_ERR,
		IB_QP_STATE,
		0
	},
	{
		IB_QPS_RTR,
		IB_QPS_ERR,
		IB_QP_STATE,
		0
	},
	{
		IB_QPS_RTS,
		IB_QPS_ERR,
		IB_QP_STATE,
		0
	},
	{
		IB_QPS_SQD,
		IB_QPS_ERR,
		IB_QP_STATE,
		0
	},
	{
		IB_QPS_ERR,
		IB_QPS_ERR,
		IB_QP_STATE,
		0
	},
	{
		IB_QPS_RESET,
		IB_QPS_RESET,
		IB_QP_STATE,
		0
	},
	{
		IB_QPS_INIT,
		IB_QPS_RESET,
		IB_QP_STATE,
		0
	},
	{
		IB_QPS_RTR,
		IB_QPS_RESET,
		IB_QP_STATE,
		0
	},
	{
		IB_QPS_RTS,
		IB_QPS_RESET,
		IB_QP_STATE,
		0
	},
	{
		IB_QPS_SQD,
		IB_QPS_RESET,
		IB_QP_STATE,
		0
	},
	{
		IB_QPS_ERR,
		IB_QPS_RESET,
		IB_QP_STATE,
		0
	}
};

void printMask(int mask)
{
	VL_MISC_TRACE(("xxxxxxxxxxxxxxxxxxxxx"));
	if (mask & IB_QP_STATE) VL_MISC_TRACE(("IB_QP_STATE"));
	if (mask & IB_QP_CUR_STATE) VL_MISC_TRACE(("IB_QP_CUR_STATE"));
	if (mask & IB_QP_EN_SQD_ASYNC_NOTIFY) VL_MISC_TRACE(("IB_QP_EN_SQD_ASYNC_NOTIFY"));
	if (mask & IB_QP_ACCESS_FLAGS) VL_MISC_TRACE(("IB_QP_ACCESS_FLAGS"));
	if (mask & IB_QP_PKEY_INDEX) VL_MISC_TRACE(("IB_QP_PKEY_INDEX"));
	if (mask & IB_QP_PORT) VL_MISC_TRACE(("IB_QP_PORT"));
	if (mask & IB_QP_QKEY) VL_MISC_TRACE(("IB_QP_QKEY"));
	if (mask & IB_QP_AV) VL_MISC_TRACE(("IB_QP_AV"));
	if (mask & IB_QP_PATH_MTU) VL_MISC_TRACE(("IB_QP_PATH_MTU"));
	if (mask & IB_QP_TIMEOUT) VL_MISC_TRACE(("IB_QP_TIMEOUT"));
	if (mask & IB_QP_RETRY_CNT) VL_MISC_TRACE(("IB_QP_RETRY_CNT"));
	if (mask & IB_QP_RNR_RETRY) VL_MISC_TRACE(("IB_QP_RNR_RETRY"));
	if (mask & IB_QP_RQ_PSN) VL_MISC_TRACE(("IB_QP_RQ_PSN"));
	if (mask & IB_QP_MAX_QP_RD_ATOMIC) VL_MISC_TRACE(("IB_QP_MAX_QP_RD_ATOMIC"));
	if (mask & IB_QP_ALT_PATH) VL_MISC_TRACE(("IB_QP_ALT_PATH"));
	if (mask & IB_QP_MIN_RNR_TIMER) VL_MISC_TRACE(("IB_QP_MIN_RNR_TIMER"));
	if (mask & IB_QP_SQ_PSN) VL_MISC_TRACE(("IB_QP_SQ_PSN"));
	if (mask & IB_QP_MAX_DEST_RD_ATOMIC) VL_MISC_TRACE(("IB_QP_MAX_DEST_RD_ATOMIC"));
	if (mask & IB_QP_PATH_MIG_STATE) VL_MISC_TRACE(("IB_QP_PATH_MIG_STATE"));
	if (mask & IB_QP_CAP) VL_MISC_TRACE(("IB_QP_CAP"));
	if (mask & IB_QP_DEST_QPN) VL_MISC_TRACE(("IB_QP_DEST_QPN"));
	VL_MISC_TRACE(("xxxxxxxxxxxxxxxxxxxxx"));
}

int next_power_of_two(
	IN	int num)
{
	int i = 0;
	while (num > (1 << i)) {
		++i;
	}
	return 1 << i;
}

void init_qp_cap(
	IN	struct resources *res,
	IN OUT	struct ib_qp_cap *cap)
{
	cap->max_recv_sge = VL_range(&res->rand_g, 1, res->device_attr.max_sge - 4);
	cap->max_send_sge = VL_range(&res->rand_g, 1, res->device_attr.max_sge - 4);
	cap->max_recv_wr = VL_range(&res->rand_g, 1, 100);//res->device_attr.max_qp_wr);  //TODO
	cap->max_send_wr = VL_range(&res->rand_g, 1, 100);// res->device_attr.max_qp_wr);

	}

enum ib_mtu get_mtu(
	IN	struct VL_random_t *rand_gen,
	IN	struct ib_port_attr *port_attr)
{
	return VL_range(rand_gen, IB_MTU_256, port_attr->max_mtu);
}

enum ib_mig_state get_path_mig_state(
	IN	struct VL_random_t *rand_gen)
{
	return IB_MIG_MIGRATED;
	
}

uint32_t get_qkey(
	IN	struct VL_random_t *rand_gen)
{
	return VL_random(rand_gen, 0xFFFFFFFF);
}

uint32_t get_psn(
	IN	struct VL_random_t *rand_gen)
{
	return VL_random(rand_gen, 0xffffff);
}

uint32_t get_dest_qp_num(
	IN	struct VL_random_t *rand_gen)
{
	return VL_random(rand_gen, 0xFFFFFF);
}

int get_qp_access_flags(
	IN	struct VL_random_t *rand_gen)
{
	int mask = 0;

	if (VL_random(rand_gen, 2))
		mask |= IB_ACCESS_REMOTE_WRITE;
	if (VL_random(rand_gen, 2))
		mask |= IB_ACCESS_REMOTE_READ;
	if (VL_random(rand_gen, 2))
		mask |= IB_ACCESS_REMOTE_ATOMIC;
	
	return mask;
}

uint16_t get_pkey_index(
	IN	struct VL_random_t *rand_gen,
	IN	struct ib_port_attr *port_attr)
{
	return VL_random(rand_gen, port_attr->pkey_tbl_len);
}

uint16_t get_en_sqd_async_notify(
	IN	struct VL_random_t *rand_gen)
{
	return VL_random(rand_gen, 2);
}

uint8_t get_sq_draining(
	IN	struct VL_random_t *rand_gen)
{
	return VL_random(rand_gen, 2);
}

uint8_t get_max_rd_atomic(
	IN	struct VL_random_t *rand_gen,
	IN	struct ib_device_attr *attr)
{
	return VL_random(rand_gen, attr->max_qp_init_rd_atom+1);
}

uint8_t get_max_dest_rd_atomic(
	IN	struct VL_random_t *rand_gen,
	IN	struct ib_device_attr *attr)
{
	return VL_random(rand_gen, attr->max_qp_rd_atom+1);
}

uint8_t get_min_rnr_timer(
	IN	struct VL_random_t *rand_gen)
{
	return VL_random(rand_gen, 0x1F);
}

uint8_t get_port_num(
	IN	struct VL_random_t *rand_gen,
	IN	int phys_port_cnt)
{
	return VL_range(rand_gen, 1, phys_port_cnt);
}

uint8_t get_timeout(
	IN	struct VL_random_t *rand_gen)
{
	return VL_random(rand_gen, 0x20);
}

uint8_t get_retry_cnt(
	IN	struct VL_random_t *rand_gen)
{
	return VL_random(rand_gen, 0x8);
}

uint8_t get_rnr_retry(
	IN	struct VL_random_t *rand_gen)
{
	return VL_random(rand_gen, 0x8);
}

void init_qp_attr(
	IN	struct resources *res,
	IN	int idx,
	OUT	struct ib_qp_attr *qp_attr)
{
	qp_attr->qp_state = test_vector[idx].to;

	qp_attr->cur_qp_state = test_vector[idx].from;

	qp_attr->sq_draining = get_sq_draining(&res->rand_g);
	qp_attr->en_sqd_async_notify = get_en_sqd_async_notify(&res->rand_g);
        
	qp_attr->qp_access_flags = get_qp_access_flags(&res->rand_g);

	qp_attr->pkey_index = get_pkey_index(&res->rand_g, &res->port_attr);

	qp_attr->port_num = get_port_num(&res->rand_g, res->device->phys_port_cnt);

	qp_attr->qkey = get_qkey(&res->rand_g);

	create_random_av_attr(res, &qp_attr->ah_attr);

	qp_attr->path_mtu = get_mtu(&res->rand_g, &res->port_attr);

	qp_attr->timeout = get_timeout(&res->rand_g);

	qp_attr->retry_cnt = get_retry_cnt(&res->rand_g);

	qp_attr->rnr_retry = get_rnr_retry(&res->rand_g);

	qp_attr->rq_psn = get_psn(&res->rand_g);

	qp_attr->max_rd_atomic = get_max_rd_atomic(&res->rand_g, &res->device_attr);

	qp_attr->alt_pkey_index = get_pkey_index(&res->rand_g, &res->port_attr);
	qp_attr->alt_timeout = get_timeout(&res->rand_g);
	create_random_av_attr(res, &qp_attr->alt_ah_attr);
	qp_attr->alt_port_num = VL_range(&res->rand_g, 1, res->device->phys_port_cnt);

	qp_attr->min_rnr_timer = get_min_rnr_timer(&res->rand_g);

	qp_attr->sq_psn = get_psn(&res->rand_g);

	qp_attr->max_dest_rd_atomic = get_max_dest_rd_atomic(&res->rand_g, &res->device_attr);

	init_qp_cap(res, &qp_attr->cap);

	qp_attr->dest_qp_num = get_dest_qp_num(&res->rand_g);

	qp_attr->path_mig_state = get_path_mig_state(&res->rand_g);
}

void cleanup_mask(
	IN	enum ib_qp_type qp_type,
	IN OUT	int* mask)
{
	if (qp_type == IB_QPT_UD) {
		*mask &= ~IB_QP_ACCESS_FLAGS;
		*mask &= ~IB_QP_DEST_QPN;
		*mask &= ~IB_QP_AV;
		*mask &= ~IB_QP_PATH_MTU;
		*mask &= ~IB_QP_ALT_PATH;
		*mask &= ~IB_QP_RQ_PSN;

	} else
		*mask &= ~IB_QP_QKEY;
	if (qp_type != IB_QPT_RC) {
		*mask &= ~IB_QP_TIMEOUT;
		*mask &= ~IB_QP_RETRY_CNT;
		*mask &= ~IB_QP_RNR_RETRY;
		*mask &= ~IB_QP_MIN_RNR_TIMER;
		*mask &= ~IB_QP_MAX_DEST_RD_ATOMIC;
		*mask &= ~IB_QP_MAX_QP_RD_ATOMIC;
	}
}

int my_query_qp(
	IN	struct ib_qp *qp,
	IN	struct ib_qp_attr *attr,
	IN	int mask)
{
	struct ib_qp_attr 	query_attr;
	struct ib_qp_init_attr	query_init_attr;
	int 			rc;

	rc = ib_query_qp(qp, &query_attr, 0xFFFFFFFF, &query_init_attr);
	CHECK_VALUE("ib_query_qp", rc, 0, return -1);

	if (mask & IB_QP_STATE) {
		CHECK_VALUE("qp_state", query_attr.qp_state, attr->qp_state, return -1);
	}

	if (mask & IB_QP_CUR_STATE) {
		CHECK_VALUE("cur_qp_state", query_attr.cur_qp_state, attr->qp_state, return -1);
	}

	if (mask & IB_QP_ACCESS_FLAGS) {
		if (!((query_attr.qp_access_flags == (attr->qp_access_flags & IB_ACCESS_REMOTE_WRITE)) ||
		      (query_attr.qp_access_flags == attr->qp_access_flags))) {
			VL_DATA_ERR(("Error in qp_access_flags, expected value %d, actual value %d",
				     attr->qp_access_flags, query_attr.qp_access_flags));
			return -1;
		}
	}

	if (mask & IB_QP_PKEY_INDEX) {
		CHECK_VALUE("pkey_index", query_attr.pkey_index, attr->pkey_index, return -1);
	}

	if (mask & IB_QP_PORT) {
		CHECK_VALUE("port_num", query_attr.port_num, attr->port_num, return -1);
	}

	if (mask & IB_QP_QKEY) {
		CHECK_VALUE("qkey", query_attr.qkey, attr->qkey, return -1);
	}

	if (mask & IB_QP_AV) {
		CHECK_VALUE("AV dlid", query_attr.ah_attr.dlid, attr->ah_attr.dlid, return -1);
		CHECK_VALUE("AV ah_flags", query_attr.ah_attr.ah_flags, attr->ah_attr.ah_flags, return -1);
/*		CHECK_VALUE("AV port_num", query_attr.ah_attr.port_num, attr->ah_attr.port_num, return -1); */
		CHECK_VALUE("AV sl", query_attr.ah_attr.sl, attr->ah_attr.sl, return -1);
		CHECK_VALUE("AV src_path_bits", query_attr.ah_attr.src_path_bits, attr->ah_attr.src_path_bits, return -1);
		CHECK_VALUE("AV static_rate", query_attr.ah_attr.static_rate, !!attr->ah_attr.static_rate, return -1);
		if (query_attr.ah_attr.ah_flags == IB_AH_GRH) {
			int i;
			
			CHECK_VALUE("AV grh.flow_label", query_attr.ah_attr.grh.flow_label, attr->ah_attr.grh.flow_label, return -1);
			CHECK_VALUE("AV grh.hop_limit", query_attr.ah_attr.grh.hop_limit, attr->ah_attr.grh.hop_limit, return -1);
			CHECK_VALUE("AV grh.sgid_index", query_attr.ah_attr.grh.sgid_index, attr->ah_attr.grh.sgid_index, return -1);
			CHECK_VALUE("AV grh.traffic_class", query_attr.ah_attr.grh.traffic_class, attr->ah_attr.grh.traffic_class, return -1);
			
			for (i = 0; i < 16; ++i) {
				CHECK_VALUE("AV grh.dgid", query_attr.ah_attr.grh.dgid.raw[i], attr->ah_attr.grh.dgid.raw[i], return -1);
			}
		}

	}

	if (mask & IB_QP_PATH_MTU) {
		CHECK_VALUE("path_mtu", query_attr.path_mtu, attr->path_mtu, return -1);
	}

	if (mask & IB_QP_TIMEOUT) {
		if (query_attr.timeout < attr->timeout) {
			VL_DATA_ERR(("invalid timeout, requested %d actual %d", attr->timeout, query_attr.timeout));
			return -1;
		}
	}

	if (mask & IB_QP_RETRY_CNT) {
		CHECK_VALUE("retry_cnt", query_attr.retry_cnt, attr->retry_cnt, return -1);
	}

	if (mask & IB_QP_RNR_RETRY) {
		CHECK_VALUE("rnr_retry", query_attr.rnr_retry, attr->rnr_retry, return -1);
	}

	if (mask & IB_QP_RQ_PSN) {
		CHECK_VALUE("rq_psn", query_attr.rq_psn, attr->rq_psn, return -1);
	}

	if (mask & IB_QP_MAX_QP_RD_ATOMIC) {
		CHECK_VALUE("max_rd_atomic", query_attr.max_rd_atomic, next_power_of_two(attr->max_rd_atomic), return -1);
	}

	if (mask & IB_QP_ALT_PATH) {
		CHECK_VALUE("alt dlid", query_attr.alt_ah_attr.dlid, attr->alt_ah_attr.dlid, return -1);
		CHECK_VALUE("alt ah_flags", query_attr.alt_ah_attr.ah_flags, attr->alt_ah_attr.ah_flags, return -1);
		CHECK_VALUE("alt port_num", query_attr.alt_ah_attr.port_num, attr->alt_port_num, return -1);
		CHECK_VALUE("alt sl", query_attr.alt_ah_attr.sl, attr->alt_ah_attr.sl, return -1);
		CHECK_VALUE("alt src_path_bits", query_attr.alt_ah_attr.src_path_bits, attr->alt_ah_attr.src_path_bits, return -1);
		CHECK_VALUE("alt static_rate", query_attr.alt_ah_attr.static_rate, !!attr->alt_ah_attr.static_rate, return -1);

		if (query_attr.alt_ah_attr.ah_flags == IB_AH_GRH) {
			int i;
			
			CHECK_VALUE("alt grh.flow_label", query_attr.alt_ah_attr.grh.flow_label, attr->alt_ah_attr.grh.flow_label, return -1);
			CHECK_VALUE("alt grh.hop_limit", query_attr.alt_ah_attr.grh.hop_limit, attr->alt_ah_attr.grh.hop_limit, return -1);
			CHECK_VALUE("alt grh.sgid_index", query_attr.alt_ah_attr.grh.sgid_index, attr->alt_ah_attr.grh.sgid_index, return -1);
			CHECK_VALUE("alt grh.traffic_class", query_attr.alt_ah_attr.grh.traffic_class, attr->alt_ah_attr.grh.traffic_class, return -1);
			
			for (i = 0; i < 16; ++i) {
				CHECK_VALUE("alt grh.dgid", query_attr.alt_ah_attr.grh.dgid.raw[i], attr->alt_ah_attr.grh.dgid.raw[i], return -1);
			}
		}

		CHECK_VALUE("alt_pkey_index", query_attr.alt_pkey_index, attr->alt_pkey_index, return -1);

		if (query_attr.alt_timeout < attr->alt_timeout) {
			VL_DATA_ERR(("invalid alt_timeout, requested %d actual %d", attr->alt_timeout, query_attr.alt_timeout));
			return -1;
		}
	}

	if (mask & IB_QP_MIN_RNR_TIMER) {
		CHECK_VALUE("min_rnr_timer", query_attr.min_rnr_timer, attr->min_rnr_timer, return -1);
	}

	if (mask & IB_QP_SQ_PSN) {
		CHECK_VALUE("sq_psn", query_attr.sq_psn, attr->sq_psn, return -1);
	}

	if (mask & IB_QP_MAX_DEST_RD_ATOMIC) {
		CHECK_VALUE("max_dest_rd_atomic", query_attr.max_dest_rd_atomic, next_power_of_two(attr->max_dest_rd_atomic), return -1);
	}

	if (mask & IB_QP_PATH_MIG_STATE) {
		CHECK_VALUE("path_mig_state", query_attr.path_mig_state, attr->path_mig_state, return -1);
	}

	if (mask & IB_QP_CAP) {
		CHECK_VALUE("cap.max_inline_data", query_attr.cap.max_inline_data, attr->cap.max_inline_data, return -1);
		CHECK_VALUE("cap.max_recv_sge", query_attr.cap.max_recv_sge, attr->cap.max_recv_sge, return -1);
		CHECK_VALUE("cap.max_recv_wr", query_attr.cap.max_recv_wr, attr->cap.max_recv_wr, return -1);
		CHECK_VALUE("cap.max_send_sge", query_attr.cap.max_send_sge, attr->cap.max_send_sge, return -1);
	}

	if (mask & IB_QP_DEST_QPN) {
		CHECK_VALUE("dest_qp_num", query_attr.dest_qp_num, attr->dest_qp_num, return -1);
	}

	return 0;
}

int my_modify_qp(
	IN	struct VL_random_t *rand_gen,
	IN	struct ib_qp *qp,
	IN	enum ib_qp_type type,
	IN	int port,
	IN	enum ib_qp_state state,
	IN	int connect_to_self)
{
	enum ib_qp_attr_mask mask;
	int rc;

	if (state >= IB_QPS_INIT) {
		struct ib_qp_attr attr = {
			.qp_state        = IB_QPS_INIT,
			.pkey_index      = 0,
			.port_num        = port,
			.qkey = VL_random(rand_gen, 0xFFFFFFFF),
			.qp_access_flags = 0

		};
		mask = IB_QP_STATE | IB_QP_PKEY_INDEX | IB_QP_PORT;
		if (type == IB_QPT_UD) {
			mask |= IB_QP_QKEY;
		} else {
			mask |= IB_QP_ACCESS_FLAGS;
		}

		rc = ib_modify_qp(qp, &attr, mask);
		CHECK_VALUE("ib_modify_qp", rc , 0, return -1);
	}

	if (state >= IB_QPS_RTR) {
		struct ib_qp_attr attr = {
			.qp_state		= IB_QPS_RTR,
			.path_mtu		= IB_MTU_1024,
			.dest_qp_num		= connect_to_self ? qp->qp_num : VL_random(rand_gen, 0xFFFFFFFF),
			.rq_psn 		= VL_random(rand_gen, 0xFFFFFFFF),
			.max_dest_rd_atomic	= 1,
			.min_rnr_timer		= 12,
			.ah_attr		= {
				.ah_flags	= 0,
				.dlid		= VL_random(rand_gen, 0xFFFF),
				.sl		= 0,
				.src_path_bits	= 0,
				.port_num	= port
			}
		};
		mask = IB_QP_STATE;
		if (type == IB_QPT_RC) {
			mask |= IB_QP_AV | IB_QP_PATH_MTU | IB_QP_DEST_QPN | IB_QP_RQ_PSN | IB_QP_MAX_DEST_RD_ATOMIC |
				IB_QP_MIN_RNR_TIMER;
		} else if (type == IB_QPT_UC) {
			mask |= IB_QP_AV | IB_QP_PATH_MTU | IB_QP_DEST_QPN | IB_QP_RQ_PSN;
		}
		rc = ib_modify_qp(qp, &attr, mask);
		CHECK_VALUE("ib_modify_qp", rc , 0, return -2);
	}

	if (state >= IB_QPS_RTS) {
		struct ib_qp_attr attr = {
			.qp_state 	= IB_QPS_RTS,
			.timeout 	= 14,
			.retry_cnt 	= 7,
			.rnr_retry 	= 7,
			.sq_psn 	= VL_random(rand_gen, 0xFFFFFFFF),
			.max_rd_atomic  = 1
		};
		mask = IB_QP_STATE | IB_QP_SQ_PSN;
		if (type == IB_QPT_RC)
			mask |= IB_QP_TIMEOUT | IB_QP_RETRY_CNT | IB_QP_RNR_RETRY | IB_QP_MAX_QP_RD_ATOMIC;
		rc = ib_modify_qp(qp, &attr, mask);
		CHECK_VALUE("ib_modify_qp", rc , 0, return -3);
	}

	if (state >= IB_QPS_SQD) {
		struct ib_qp_attr attr = {
			.qp_state 	= IB_QPS_SQD
		};
		mask = IB_QP_STATE;
		rc = ib_modify_qp(qp, &attr, mask);
		CHECK_VALUE("ib_modify_qp", rc , 0, return -4);
	}


	return 0;
}

/* ib_create_qp ib_destroy_qp */
int qp_1(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_pd 		*pd = ERR_PTR(-EINVAL);
	struct ib_cq		*cq = ERR_PTR(-EINVAL);
	struct ib_qp		*qp = ERR_PTR(-EINVAL);
	enum ib_qp_type		type;
	int			result = -1;
	int			rc;

	TEST_CASE("ib_create_qp ib_destroy_qp");

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	cq = ib_create_cq(res->device, cq_comp_handler, component_event_handler, NULL, 1, 0);
	CHECK_PTR("ib_create_cq", !IS_ERR(cq), goto cleanup);

	for (type = IB_QPT_RC; type <= IB_QPT_UD; type++) {
		memset(&res->attributes.qp_init_attr, 0, sizeof(res->attributes.qp_init_attr));
		init_qp_cap(res, &res->attributes.qp_init_attr.cap);
		res->attributes.qp_init_attr.recv_cq = cq;
		res->attributes.qp_init_attr.send_cq = cq;
		res->attributes.qp_init_attr.qp_type = type;
		res->attributes.qp_init_attr.event_handler = component_event_handler;

		res->attributes.qp_init_attr.port_num = VL_range(&res->rand_g, 1, res->device->phys_port_cnt);
	
		qp = ib_create_qp(pd, &res->attributes.qp_init_attr);
		CHECK_PTR("ib_create_qp", !IS_ERR(qp), VL_DATA_ERR(("error %ld type = %d",PTR_ERR(qp), type)); goto cleanup);

		rc = ib_destroy_qp(qp);
		CHECK_VALUE("ib_destroy_qp", rc, 0, goto cleanup);
		qp = ERR_PTR(-EINVAL);
	}

	PASSED;

	TEST_CASE("create only with SQ");

	for (type = IB_QPT_RC; type <= IB_QPT_UD; type++) {
		memset(&res->attributes.qp_init_attr, 0, sizeof(res->attributes.qp_init_attr));
		init_qp_cap(res, &res->attributes.qp_init_attr.cap);
		res->attributes.qp_init_attr.cap.max_recv_sge = 0;
		res->attributes.qp_init_attr.cap.max_recv_wr = 0;
		res->attributes.qp_init_attr.recv_cq = cq;
		res->attributes.qp_init_attr.send_cq = cq;
		res->attributes.qp_init_attr.qp_type = type;
		res->attributes.qp_init_attr.event_handler = component_event_handler;

		res->attributes.qp_init_attr.port_num = VL_range(&res->rand_g, 1, res->device->phys_port_cnt);
	
		qp = ib_create_qp(pd, &res->attributes.qp_init_attr);
		CHECK_PTR("ib_create_qp", !IS_ERR(qp), VL_DATA_ERR(("error %ld type = %d",PTR_ERR(qp), type)); goto cleanup);

		rc = ib_destroy_qp(qp);
		CHECK_VALUE("ib_destroy_qp", rc, 0, goto cleanup);
		qp = ERR_PTR(-EINVAL);
	}

	PASSED;

	TEST_CASE("create only with RQ");

	for (type = IB_QPT_RC; type <= IB_QPT_UD; type++) {
		memset(&res->attributes.qp_init_attr, 0, sizeof(res->attributes.qp_init_attr));
		init_qp_cap(res, &res->attributes.qp_init_attr.cap);
		res->attributes.qp_init_attr.cap.max_send_sge = 0;
		res->attributes.qp_init_attr.cap.max_send_wr = 0;
		res->attributes.qp_init_attr.recv_cq = cq;
		res->attributes.qp_init_attr.send_cq = cq;
		res->attributes.qp_init_attr.qp_type = type;
		res->attributes.qp_init_attr.event_handler = component_event_handler;

		res->attributes.qp_init_attr.port_num = VL_range(&res->rand_g, 1, res->device->phys_port_cnt);
	
		qp = ib_create_qp(pd, &res->attributes.qp_init_attr);
		CHECK_PTR("ib_create_qp", !IS_ERR(qp), VL_DATA_ERR(("error %ld type = %d",PTR_ERR(qp), type)); goto cleanup);

		rc = ib_destroy_qp(qp);
		CHECK_VALUE("ib_destroy_qp", rc, 0, goto cleanup);
		qp = ERR_PTR(-EINVAL);
	}

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

/* ib_create_qp ib_modify_qp */
int qp_2(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_pd 		*pd = ERR_PTR(-EINVAL);
	struct ib_cq		*cq = ERR_PTR(-EINVAL);
	struct ib_qp		*qp = ERR_PTR(-EINVAL);
	struct ib_qp_attr	qp_modify_attr;
	enum ib_qp_type		type;
	enum test_type		test;
	int			mask;
	int			i;
	int			result = -1;
	int			rc;

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	cq = ib_create_cq(res->device, cq_comp_handler, component_event_handler, NULL, 1, 0);
	CHECK_PTR("ib_create_cq", !IS_ERR(cq), goto cleanup);

	for (test = REQUIRED_ATTR; test <= OPTIONAL_ATTR; test++) {
		for (type = IB_QPT_RC; type <= IB_QPT_UD; type++) {
			for (i = 0; i < sizeof(test_vector) / sizeof(struct test_vector_t); i++) {
				memset(&res->attributes.qp_init_attr, 0, sizeof(res->attributes.qp_init_attr));
				init_qp_cap(res, &res->attributes.qp_init_attr.cap);
				res->attributes.qp_init_attr.recv_cq = cq;
				res->attributes.qp_init_attr.send_cq = cq;
				res->attributes.qp_init_attr.qp_type = type;
				res->attributes.qp_init_attr.event_handler = component_event_handler;
/*			
				VL_MISC_TRACE(("test %d QP %s modify from %s to %s", test, VL_ib_qp_type_str(type), 
				       VL_ib_qp_state_str(test_vector[i].from), VL_ib_qp_state_str(test_vector[i].to)));
*/
				qp = ib_create_qp(pd, &res->attributes.qp_init_attr);
				CHECK_PTR("ib_create_qp", !IS_ERR(qp), VL_DATA_ERR(("error %ld type = %d",PTR_ERR(qp), type)); goto cleanup);

				rc = my_modify_qp(&res->rand_g, qp, type, VL_range(&res->rand_g, 1, res->device->phys_port_cnt), 
						  test_vector[i].from, 0);
				CHECK_VALUE("my_modify_qp", rc, 0, goto cleanup);

				mask = 0;
				switch (test) {
				case OPTIONAL_ATTR:
					mask = test_vector[i].optional_attr;
				case REQUIRED_ATTR:
					mask |= test_vector[i].required_attr;
				}

				cleanup_mask(type, &mask);

				if (mask & IB_QP_PATH_MIG_STATE)
					mask &= ~IB_QP_PATH_MIG_STATE;

				init_qp_attr(res, i, &qp_modify_attr);

				rc = ib_modify_qp(qp, &qp_modify_attr, mask);
				CHECK_VALUE("ib_modify_qp", rc, 0, printMask(mask); VL_DATA_ERR(("port %d", qp_modify_attr.port_num)); goto cleanup);

				if (test_vector[i].to != IB_QPS_RESET) { // query in RESET not supported
					rc = my_query_qp(qp, &qp_modify_attr, mask);
					CHECK_VALUE("my_query_qp", rc, 0, goto cleanup);
				}

				rc = ib_destroy_qp(qp);
				CHECK_VALUE("ib_destroy_qp", rc, 0, goto cleanup);
				qp = ERR_PTR(-EINVAL);
			}
		}
	}

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

/* query init cap */
int qp_3(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_pd 		*pd = ERR_PTR(-EINVAL);
	struct ib_cq		*cq = ERR_PTR(-EINVAL);
	struct ib_srq		*srq = ERR_PTR(-EINVAL);
	struct ib_qp		*qp = ERR_PTR(-EINVAL);
	enum ib_qp_type		type;
	struct ib_qp_attr 	query_attr;
	struct ib_qp_init_attr	query_init_attr;
	int			result = -1;
	int			rc;

	TEST_CASE("query init cap");

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	cq = ib_create_cq(res->device, cq_comp_handler, component_event_handler, NULL, 1, 0);
	CHECK_PTR("ib_create_cq", !IS_ERR(cq), goto cleanup);

	res->attributes.srq_init_attr.event_handler = component_event_handler;
	init_srq_cap(config, res, &res->attributes.srq_init_attr);

	srq = ib_create_srq(pd, &res->attributes.srq_init_attr);
	CHECK_PTR("ib_create_srq", !IS_ERR(srq), goto cleanup);

	for (type = IB_QPT_RC; type <= IB_QPT_UD; type++) {
		memset(&res->attributes.qp_init_attr, 0, sizeof(res->attributes.qp_init_attr));
		init_qp_cap(res, &res->attributes.qp_init_attr.cap);
		res->attributes.qp_init_attr.recv_cq = cq;
		res->attributes.qp_init_attr.send_cq = cq;
		res->attributes.qp_init_attr.srq = srq;
		res->attributes.qp_init_attr.qp_type = type;
		res->attributes.qp_init_attr.event_handler = component_event_handler;

		qp = ib_create_qp(pd, &res->attributes.qp_init_attr);
		CHECK_PTR("ib_create_qp", !IS_ERR(qp), VL_DATA_ERR(("error %ld type = %d",PTR_ERR(qp), type)); goto cleanup);

		rc = my_modify_qp(&res->rand_g, qp, type, VL_range(&res->rand_g, 1, res->device->phys_port_cnt), IB_QPS_INIT, 0);
		CHECK_VALUE("my_modify_qp", rc, 0, goto cleanup);

		rc = ib_query_qp(qp, &query_attr, 0xFFFFFFFF, &query_init_attr);
		CHECK_VALUE("ib_query_qp", rc, 0, goto cleanup);

                CHECK_VALUE("max_inline_data", query_init_attr.cap.max_inline_data, res->attributes.qp_init_attr.cap.max_inline_data, goto cleanup);
		CHECK_VALUE("max_recv_sge", query_init_attr.cap.max_recv_sge, res->attributes.qp_init_attr.cap.max_recv_sge, goto cleanup);
		CHECK_VALUE("max_recv_wr", query_init_attr.cap.max_recv_wr, res->attributes.qp_init_attr.cap.max_recv_wr, goto cleanup);
		CHECK_VALUE("max_send_sge", query_init_attr.cap.max_send_sge, res->attributes.qp_init_attr.cap.max_send_sge, goto cleanup);
		CHECK_VALUE("max_send_wr", query_init_attr.cap.max_send_wr, res->attributes.qp_init_attr.cap.max_send_wr, goto cleanup);

		CHECK_VALUE("qp_context", (unsigned long)query_init_attr.qp_context, (unsigned long)res->attributes.qp_init_attr.qp_context, goto cleanup);
		CHECK_VALUE("qp_type", query_init_attr.qp_type, res->attributes.qp_init_attr.qp_type, goto cleanup);
		CHECK_VALUE("recv_cq", (unsigned long)query_init_attr.recv_cq, (unsigned long)res->attributes.qp_init_attr.recv_cq, goto cleanup);
		CHECK_VALUE("send_cq", (unsigned long)query_init_attr.send_cq, (unsigned long)res->attributes.qp_init_attr.send_cq, goto cleanup);
		CHECK_VALUE("sq_sig_all", query_init_attr.sq_sig_type, res->attributes.qp_init_attr.sq_sig_type, goto cleanup);
		CHECK_VALUE("srq", (unsigned long)query_init_attr.srq, (unsigned long)res->attributes.qp_init_attr.srq, goto cleanup);

		rc = ib_destroy_qp(qp);
		CHECK_VALUE("ib_destroy_qp", rc, 0, goto cleanup);
		qp = ERR_PTR(-EINVAL);
	}

	PASSED;

	result = 0;

cleanup:

	if (qp != NULL && !IS_ERR(qp)) {
		rc = ib_destroy_qp(qp);
		CHECK_VALUE("ib_destroy_qp", rc, 0, return -1);
	}

	if (!IS_ERR(srq)) {
		rc= ib_destroy_srq(srq);
		CHECK_VALUE("ib_destroy_srq", rc, 0, return -1);
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

/* APM */
int qp_4(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_pd 		*pd = ERR_PTR(-EINVAL);
	struct ib_cq		*cq = ERR_PTR(-EINVAL);
	struct ib_qp		*qp = ERR_PTR(-EINVAL);
	struct ib_qp_attr	qp_modify_attr;
	struct ib_qp_attr 	query_attr;
	struct ib_qp_init_attr	query_init_attr;
	enum ib_qp_type		type;
	int			mask;
	int			timeout;
	int			i;
	int			result = -1;
	int			rc;

	TEST_CASE("APM");

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	cq = ib_create_cq(res->device, cq_comp_handler, component_event_handler, NULL, 1, 0);
	CHECK_PTR("ib_create_cq", !IS_ERR(cq), goto cleanup);

	for (type = IB_QPT_RC; type <= IB_QPT_UC; type++) {
		for (i = 0; i < 3; ++i) {
			init_qp_cap(res, &res->attributes.qp_init_attr.cap);
			res->attributes.qp_init_attr.recv_cq = cq;
			res->attributes.qp_init_attr.send_cq = cq;
			res->attributes.qp_init_attr.qp_type = type;
			res->attributes.qp_init_attr.event_handler = component_event_handler;
	
			qp = ib_create_qp(pd, &res->attributes.qp_init_attr);
			CHECK_PTR("ib_create_qp", !IS_ERR(qp), VL_DATA_ERR(("error %ld type = %d",PTR_ERR(qp), type)); goto cleanup);

			switch(i) {
			case 0: /* RTR -> RTS */
				rc = my_modify_qp(&res->rand_g, qp, type, VL_range(&res->rand_g, 1, res->device->phys_port_cnt), IB_QPS_RTR, 0);
				CHECK_VALUE("my_modify_qp", rc, 0, goto cleanup);

				init_qp_attr(res, 3, &qp_modify_attr);

				mask = IB_QP_STATE | IB_QP_TIMEOUT | IB_QP_RETRY_CNT | IB_QP_RNR_RETRY | 
					IB_QP_SQ_PSN | IB_QP_MAX_QP_RD_ATOMIC | IB_QP_PATH_MIG_STATE;

				cleanup_mask(type, &mask);

				qp_modify_attr.path_mig_state = IB_MIG_REARM;

				rc = ib_modify_qp(qp, &qp_modify_attr, mask);
				CHECK_VALUE("ib_modify_qp", rc, 0, printMask(mask); goto cleanup);

				break;
			case 1: /* RTS -> RTS */
				rc = my_modify_qp(&res->rand_g, qp, type, VL_range(&res->rand_g, 1, res->device->phys_port_cnt), IB_QPS_RTS, 0);
				CHECK_VALUE("my_modify_qp", rc, 0, goto cleanup);

				qp_modify_attr.qp_state = IB_QPS_RTS;

				mask = IB_QP_STATE | IB_QP_PATH_MIG_STATE;

				cleanup_mask(type, &mask);

				qp_modify_attr.path_mig_state = IB_MIG_REARM;

				rc = ib_modify_qp(qp, &qp_modify_attr, mask);
				CHECK_VALUE("ib_modify_qp", rc, 0, printMask(mask); goto cleanup);

				break;
			case 2: /* SQD -> RTS */
				rc = my_modify_qp(&res->rand_g, qp, type, VL_range(&res->rand_g, 1, res->device->phys_port_cnt), IB_QPS_SQD, 0);
				CHECK_VALUE("my_modify_qp", rc, 0, goto cleanup);

				timeout = 10;
				do {
					rc = ib_query_qp(qp, &query_attr, 0xFFFFFFFF, &query_init_attr);
					CHECK_VALUE("ib_query_qp", rc, 0, goto cleanup);
					timeout--;
					msleep(10);
				} while (query_attr.qp_state != IB_QPS_SQD && timeout);
				if (!timeout) {
					VL_DATA_ERR(("QP didn't reach the SQD state"));
					goto cleanup;
				}

				qp_modify_attr.qp_state = IB_QPS_RTS;

				mask = IB_QP_STATE | IB_QP_PATH_MIG_STATE;

				cleanup_mask(type, &mask);

				qp_modify_attr.path_mig_state = IB_MIG_REARM;

				rc = ib_modify_qp(qp, &qp_modify_attr, mask);
				CHECK_VALUE("ib_modify_qp", rc, 0, printMask(mask); goto cleanup);

				break;
			}

			rc = ib_destroy_qp(qp);
			CHECK_VALUE("ib_destroy_qp", rc, 0, goto cleanup);
			qp = ERR_PTR(-EINVAL);
		}
	}

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

int test_qp(
	IN	struct config_t* config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	RUN_TEST(1, qp_1);
	RUN_TEST(2, qp_2);
	RUN_TEST(3, qp_3);
	RUN_TEST(4, qp_4);

	return 0;
}
