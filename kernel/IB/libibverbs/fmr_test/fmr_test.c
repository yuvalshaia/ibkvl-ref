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
 * $Id: basic_module.c 12392 2011-05-03 07:52:28Z sharonc $ 
 * 
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/list.h>		/* for list handling */
#include <linux/slab.h>         /* kmalloc */
#include <linux/vmalloc.h>	/* for vmalloc */
#include <linux/sched.h>	/* for yield */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
	#include <asm/semaphore.h>	/* for semaphore */
#else
	#include <linux/semaphore.h> 
#endif
#include <asm/uaccess.h>	/* for copy_from_user */

#include <vl_ib_verbs.h>

#include <vl.h>
#include <vl_gen2k_str.h>
#include "fmr_test.h"
#include "types.h"


#define RELEASE_SEMAPHORE up(&gdata[thread_pair_idx].sems[my_idx_in_pair])
#define ACQUIRE_SEMAPHORE down(&gdata[thread_pair_idx].sems[peers_idx_in_pair]); 			\
			    if (gdata[thread_pair_idx].completed[peers_idx_in_pair] == -1) {		\
				VL_MISC_ERR(("thread(%d) the other thread was failed", thread));	\
				ret = -1;								\
				goto exit_thread_func;							\
			    }

#ifndef MIN
#define MIN(a, b) (((a) > (b)) ? (b) : (a))
#endif


#define MAX_NUM_PREALLOCED_PAGES	10
#define REQ_CQE_NUM			4
#define MAX_COMP_TIMEOUT		200	/* max time in msec to poll for completion */

/*****************************************************************************
* Functions
*****************************************************************************/
static void test_add_one(
	IN	struct ib_device *device);

static void test_remove_one(
	IN	struct ib_device *device);


/*****************************************************************************
* Global Variables
*****************************************************************************/
static const char *VL_driver_generic_name = "fmr_test:";
static volatile int exit_val = -1;
static struct resources_t res;
static struct config_t config;
static struct thread_global_data_t *gdata = NULL;


struct ib_client test_client = {
	.name   = "fmr_test",
	.add    = test_add_one,
	.remove = test_remove_one
};


/***************************************
* Function: modify_qp_state_to_RESET
****************************************
* Description: change the QP state to RESET
***************************************/
static int modify_qp_state_to_RESET(
	IN		int thread,
	IN		struct ib_qp *qp_hndl,
	OUT		struct modify_qp_props_t *modify_qp_props)
{
	int rc;


	/* modify the QP to RESET */
	memset(&modify_qp_props->qp_attr, 0, sizeof(modify_qp_props->qp_attr));

	/**********************************************
	* modify the QP to RESET
	**********************************************/
	modify_qp_props->qp_attr_mask = 0;

	
	modify_qp_props->qp_attr.qp_state = IB_QPS_RESET;
	modify_qp_props->qp_attr_mask |= IB_QP_STATE;

	rc = ib_modify_qp(qp_hndl, &modify_qp_props->qp_attr, modify_qp_props->qp_attr_mask);
	if (rc) {
		VL_DATA_ERR(("thread(%d) ib_modify_qp to RESET failed", thread));
		return rc;
	}

	return 0;
}


/***************************************
* Function: modify_qp_state_to_RTS
****************************************
* Description: change the QP state from RESET to RTS
***************************************/
static int modify_qp_state_to_RTS(
	IN		int thread, 
	IN		struct ib_qp *qp_hndl,
	IN		u8 my_port, 
	IN		u16 dlid, 
	IN		u32 dest_qp_num, 
	INOUT		u32 *psn,
	OUT		struct modify_qp_props_t *modify_qp_props)
{
	int rc;


	memset(&modify_qp_props->qp_attr, 0, sizeof(modify_qp_props->qp_attr));

	/**********************************************
	* modify the QP to INIT
	**********************************************/
	modify_qp_props->qp_attr_mask = 0;


	modify_qp_props->qp_attr.qp_state = IB_QPS_INIT;
	modify_qp_props->qp_attr_mask |= IB_QP_STATE;

	modify_qp_props->qp_attr.pkey_index = 0;
	modify_qp_props->qp_attr_mask |= IB_QP_PKEY_INDEX;

	modify_qp_props->qp_attr.port_num = my_port;
	modify_qp_props->qp_attr_mask |= IB_QP_PORT;

	modify_qp_props->qp_attr.qp_access_flags = IB_ACCESS_REMOTE_WRITE | IB_ACCESS_REMOTE_READ;
	modify_qp_props->qp_attr_mask |= IB_QP_ACCESS_FLAGS;

	rc = ib_modify_qp(qp_hndl, &modify_qp_props->qp_attr, modify_qp_props->qp_attr_mask);
	if (rc) {
		VL_DATA_ERR(("thread(%d) ib_modify_qp from RESET to INIT failed", thread));
		return rc;
	}

	/**********************************************
	* modify the QP to RTR
	**********************************************/
	modify_qp_props->qp_attr_mask = 0;


	modify_qp_props->qp_attr.qp_state = IB_QPS_RTR;
	modify_qp_props->qp_attr_mask |= IB_QP_STATE;

	modify_qp_props->qp_attr.ah_attr.sl = 0;
	modify_qp_props->qp_attr.ah_attr.ah_flags = 0;
	modify_qp_props->qp_attr.ah_attr.dlid = dlid;
	modify_qp_props->qp_attr.ah_attr.static_rate = 2; /* 1x */
	modify_qp_props->qp_attr.ah_attr.src_path_bits = 0; 
	modify_qp_props->qp_attr_mask |= IB_QP_AV;

	modify_qp_props->qp_attr.path_mtu = IB_MTU_256; /* this MTU is always supported */
	modify_qp_props->qp_attr_mask |= IB_QP_PATH_MTU;

	modify_qp_props->qp_attr.rq_psn = *psn;
	modify_qp_props->qp_attr_mask |= IB_QP_RQ_PSN;

	modify_qp_props->qp_attr.max_dest_rd_atomic = 1;
	modify_qp_props->qp_attr_mask |= IB_QP_MAX_DEST_RD_ATOMIC;

	modify_qp_props->qp_attr.dest_qp_num = dest_qp_num;
	modify_qp_props->qp_attr_mask |= IB_QP_DEST_QPN;

	modify_qp_props->qp_attr.min_rnr_timer = 0; 
	modify_qp_props->qp_attr_mask |= IB_QP_MIN_RNR_TIMER;

	rc = ib_modify_qp(qp_hndl, &modify_qp_props->qp_attr, modify_qp_props->qp_attr_mask);
	if (rc) {
		VL_DATA_ERR(("thread(%d) ib_modify_qp from INIT to RTR failed", thread));
		return rc;
	}
	/**********************************************
	* modify the QP to RTS
	**********************************************/
	modify_qp_props->qp_attr_mask = 0;


	modify_qp_props->qp_attr.qp_state = IB_QPS_RTS;
	modify_qp_props->qp_attr_mask |= IB_QP_STATE;

	modify_qp_props->qp_attr.sq_psn = *psn;
	modify_qp_props->qp_attr_mask |= IB_QP_SQ_PSN;

	modify_qp_props->qp_attr.timeout = 0x18;
	modify_qp_props->qp_attr_mask |= IB_QP_TIMEOUT;

	modify_qp_props->qp_attr.retry_cnt = 6;
	modify_qp_props->qp_attr_mask |= IB_QP_RETRY_CNT;

	modify_qp_props->qp_attr.rnr_retry = 0;
	modify_qp_props->qp_attr_mask |= IB_QP_RNR_RETRY;

	modify_qp_props->qp_attr.max_rd_atomic = 1;
	modify_qp_props->qp_attr_mask |= IB_QP_MAX_QP_RD_ATOMIC;

	rc = ib_modify_qp(qp_hndl, &modify_qp_props->qp_attr, modify_qp_props->qp_attr_mask);
	if (rc) {
		VL_DATA_ERR(("thread(%d) ib_modify_qp from RTR to RTS failed", thread));
		return rc;
	}

	/* increase the PSN every time we do modify QP */
	(*psn) += 100000;

	return 0;
}


/***************************************
* Function: verify_invalidated_fmr
****************************************
* Description: a function to validate that the FMR is not accessible (the keys are invalidated)
***************************************/
static int verify_invalidated_fmr(
	IN		int thread_pair_idx,
	IN		int my_idx_in_pair,
	IN		int peers_idx_in_pair,
	IN		struct validate_fmr_t *validate_data,
	IN		u32 l_key,
	IN		u32 r_key,
	IN		size_t size,
	IN		struct modify_qp_props_t *modify_qp_props,
	IN		struct invalidate_fmr_props_t *invalidate_fmr_props,
	INOUT		u32 *psn_p)
{
	struct ib_send_wr *bad_send_wr;
	unsigned long poll_total_time, poll_start_time;
	int rc;
	enum ib_wr_opcode opcode;


	/* modify the QP state to reset (because the QP may be in error state) */
	rc = modify_qp_state_to_RESET(validate_data->thread, validate_data->qp_hndl, modify_qp_props);
	if (rc)
		return rc;

	/* connect the QP to itself (loopback: RQ <--> SQ) */
	rc = modify_qp_state_to_RTS(validate_data->thread, validate_data->qp_hndl, validate_data->port, validate_data->dlid, validate_data->dest_qp_num, psn_p, modify_qp_props);
	if (rc)
		return rc;

	/* decide which opcode to post : RDMA Read  - check r_key
					 RDMA Write - check l_key */
	if (VL_random(&gdata[thread_pair_idx].rand[my_idx_in_pair], 2))
		opcode = IB_WR_RDMA_WRITE;
	else
		opcode = IB_WR_RDMA_READ;

	memset(&invalidate_fmr_props->sg_entry, 0, sizeof(invalidate_fmr_props->sg_entry));
	memset(&invalidate_fmr_props->sr_desc, 0, sizeof(invalidate_fmr_props->sr_desc));

	/* post the WR and wait for completion with error depend on the opcode */
	invalidate_fmr_props->sg_entry.addr   = gdata[thread_pair_idx].vas[my_idx_in_pair];
	invalidate_fmr_props->sg_entry.length = size;
	invalidate_fmr_props->sg_entry.lkey   = l_key;

	invalidate_fmr_props->sr_desc.next                = NULL;
	invalidate_fmr_props->sr_desc.wr_id               = validate_data->thread;
	invalidate_fmr_props->sr_desc.opcode              = opcode;
	invalidate_fmr_props->sr_desc.send_flags          = IB_SEND_SIGNALED;
	invalidate_fmr_props->sr_desc.sg_list             = &invalidate_fmr_props->sg_entry;
	invalidate_fmr_props->sr_desc.num_sge             = 1;
	invalidate_fmr_props->sr_desc.wr.rdma.remote_addr = gdata[thread_pair_idx].vas[peers_idx_in_pair];
	invalidate_fmr_props->sr_desc.wr.rdma.rkey        = r_key;

	rc = ib_post_send(validate_data->qp_hndl, &invalidate_fmr_props->sr_desc, &bad_send_wr);
	if (rc) {
		VL_DATA_ERR(("thread(%d) ib_post_send failed", validate_data->thread));
		return rc;
	}

	poll_start_time = jiffies;

	do {
		poll_total_time = jiffies - poll_start_time;

		rc = ib_poll_cq(validate_data->cq_hndl, 1, &invalidate_fmr_props->comp_desc);
		if (rc == 0) {
			if (jiffies_to_usecs(poll_total_time) < (MAX_COMP_TIMEOUT * 1000))
				yield();
			else
				break;
		}	
	} while (rc == 0);

	/* if a completion wasn't read */
	if (rc <= 0) {
		if (rc < 0)
			VL_DATA_ERR(("thread(%d) ib_poll_cq failed", validate_data->thread));
		else if (!rc) /* if we stopped polling for completion because the timeout expired */
			VL_DATA_ERR(("thread(%d): A completion wasn't found in the CQ after %d msec", validate_data->thread, MAX_COMP_TIMEOUT));
              
		return rc;
	}

	/* for RDMA_WRITE: we are using an invalid lkey, so LOC_PROT_ERR is expected
	   for RDMA_READ: we are using an invalid rkey, but we are using a loopback QP,
		   so both SQ and RQ have an error (the RQ first and then the SQ)
		   so the completion may be REM_ACCESS_ERR or WR_FLUSH_ERR, depend of the stress
		   on the HCA
	*/

	if (((opcode == IB_WR_RDMA_WRITE) && (invalidate_fmr_props->comp_desc.status != IB_WC_LOC_PROT_ERR)) ||
		((opcode == IB_WR_RDMA_READ) && (invalidate_fmr_props->comp_desc.status != IB_WC_REM_ACCESS_ERR) &&
		 (invalidate_fmr_props->comp_desc.status != IB_WC_WR_FLUSH_ERR))) {
		VL_MISC_ERR(("thread(%d) local QP number=0x%x, opcode = %s, completion status %s wasn't expected (syndrome = 0x%x)",
				validate_data->thread, validate_data->qp_hndl->qp_num,
				VL_ib_wr_opcode_str(opcode), VL_ib_wc_status_str(invalidate_fmr_props->comp_desc.status),
				invalidate_fmr_props->comp_desc.vendor_err));
		return -1;
	}

	return 0;
}

/***************************************
* Function: thread_func
***************************************/
int thread_func(void *arg)
{
	struct ib_send_wr *bad_send_wr;
	size_t size;
	unsigned long poll_start_time, poll_total_time;
	int rc;
	int thread = (int)(unsigned long)arg;
	int thread_pair_idx = thread >> 1;
	int my_idx_in_pair = thread & 1;
	int other_idx_in_pair = !my_idx_in_pair;
	int peers_idx_in_pair = (my_idx_in_pair+1) % 2;
	int ret = 0;
	int i, j;
	u8 my_port = config.ib_port, peers_port = config.ib_port;
	struct thread_rsc_attribute_t *thread_rsc_attribute = NULL;
	u_int8_t used_page_shift = 12; // todo - why to use only 12?????
	int num_of_inv_fmr;


	VL_MISC_TRACE(("thread %d was started", thread));

	thread_rsc_attribute = vmalloc(sizeof(struct thread_rsc_attribute_t));
	if (!thread_rsc_attribute) {
		VL_MEM_ERR(("thread(%d) thread resources attributes allocation failed", thread));
		ret = -1;
		goto exit_thread_func;
	}

	/* invalidate the handles */
	thread_rsc_attribute->pd_hndl = ERR_PTR(-EINVAL);
	thread_rsc_attribute->cq_hndl = ERR_PTR(-EINVAL);
	thread_rsc_attribute->qp_hndl = ERR_PTR(-EINVAL);
	thread_rsc_attribute->lb_qp_hndl = ERR_PTR(-EINVAL); /* LoopBack QP used to that the FMR is being invalidated */
	thread_rsc_attribute->buf_va = NULL;
	thread_rsc_attribute->fmr_arr = NULL;
	INIT_LIST_HEAD(&thread_rsc_attribute->fmr_hndl_list);
	thread_rsc_attribute->psn = 0;
	thread_rsc_attribute->lb_psn = 0;
	thread_rsc_attribute->last_status = IB_WC_GENERAL_ERR;
	thread_rsc_attribute->max_fmr_pages = 0;

	config.seed = VL_srand(config.seed+thread, &gdata[thread_pair_idx].rand[my_idx_in_pair]);

	/* put NULL - to mark that we didn't do any allocation (yet) to this attribute */
	thread_rsc_attribute->map.page_list = NULL;

	/* allocate the FMRs array and invalidate all the FMRs */
	size = sizeof(struct fmr_data_t) * config.num_of_fmr;
	thread_rsc_attribute->fmr_arr = vmalloc(size);
	if (!thread_rsc_attribute->fmr_arr) {
		VL_MEM_ERR(("thread(%d) allocation of %Zu bytes for fmr buffer failed", thread, size));
		ret = -1;
		goto exit_thread_func;
	}

	for (i = 0; i < config.num_of_fmr; i++)
		thread_rsc_attribute->fmr_arr[i].fmr_hndl = ERR_PTR(-EINVAL);

	/* query the HCA attributes */
	rc = ib_query_device(res.device, &thread_rsc_attribute->hca_cap);
	if (rc) {
		VL_HCA_ERR(("thread(%d) ib_query_device failed", thread));
		ret = -1;
		goto exit_thread_func;
	}

	/* query the port props */
	rc = ib_query_port(res.device, my_port, &thread_rsc_attribute->my_port_props);
	if (rc) {
		VL_HCA_ERR(("thread(%d) ib_query_port for port %u failed", thread, my_port));
		ret = -1;
		goto exit_thread_func;
	}

	if (thread_rsc_attribute->my_port_props.state != IB_PORT_ACTIVE) {
		VL_HCA_ERR(("thread(%d) Port %u state is not active: %s", thread, my_port,
				VL_ib_port_state_str(thread_rsc_attribute->my_port_props.state)));
		ret = -1;
		goto exit_thread_func;
	}

	rc = ib_query_port(res.device, peers_port, &thread_rsc_attribute->peers_port_props);
	if (rc) {
		VL_HCA_ERR(("thread(%d) ib_query_port for port %u failed", thread, peers_port));
		ret = -1;
		goto exit_thread_func;
	}

	/* allocate PD */
	thread_rsc_attribute->pd_hndl = ib_alloc_pd(res.device);
	if (IS_ERR(thread_rsc_attribute->pd_hndl)) {
		VL_HCA_ERR(("thread(%d) ib_alloc_pd failed", thread));
		ret = -1;
		goto exit_thread_func;
	}

	/* allocate FMRs */
	for (i = 0; i < config.num_of_fmr; i++) {
		if (config.bad_flow) {
			thread_rsc_attribute->rand_num = VL_random(&gdata[thread_pair_idx].rand[my_idx_in_pair], 16);

			if (my_idx_in_pair == 0) 	/* the requestor */
				/* the requestor will do only RDMA Read or RDMA Write */
				thread_rsc_attribute->acl = (thread_rsc_attribute->rand_num & 1) ? IB_ACCESS_LOCAL_WRITE : 0;
			else {				/* the responder */
				thread_rsc_attribute->acl = IB_ACCESS_LOCAL_WRITE;

				if (thread_rsc_attribute->rand_num & 2)
					thread_rsc_attribute->acl |= IB_ACCESS_REMOTE_WRITE;

				if (thread_rsc_attribute->rand_num & 4)
					thread_rsc_attribute->acl |= IB_ACCESS_REMOTE_READ;
			}
		} else {
			if (my_idx_in_pair == 0)	/* the requestor */
				thread_rsc_attribute->acl = IB_ACCESS_LOCAL_WRITE;
			else				/* the responder */
				thread_rsc_attribute->acl = IB_ACCESS_LOCAL_WRITE | IB_ACCESS_REMOTE_WRITE | IB_ACCESS_REMOTE_READ;
		}
		thread_rsc_attribute->fmr_arr[i].acl = thread_rsc_attribute->acl;

		thread_rsc_attribute->fmr_arr[i].fmr_props.max_pages = VL_range(&gdata[thread_pair_idx].rand[my_idx_in_pair], 1, MAX_NUM_PREALLOCED_PAGES);
		thread_rsc_attribute->fmr_arr[i].fmr_props.max_maps = VL_range(&gdata[thread_pair_idx].rand[my_idx_in_pair], 1, thread_rsc_attribute->hca_cap.max_map_per_fmr);
		thread_rsc_attribute->fmr_arr[i].fmr_props.page_shift = used_page_shift;

		thread_rsc_attribute->fmr_arr[i].fmr_hndl = ib_alloc_fmr(thread_rsc_attribute->pd_hndl,
									 thread_rsc_attribute->acl,
									 &thread_rsc_attribute->fmr_arr[i].fmr_props);
		if (IS_ERR(thread_rsc_attribute->fmr_arr[i].fmr_hndl)) {
			VL_MEM_ERR(("thread(%d) ib_alloc_fmr[%u] failed", thread, i));
			ret = -1;
			goto exit_thread_func;
		}
		VL_MEM_TRACE(("thread(%d) fmr[%u] was created: max_pages=%u, max_maps=%u, page_shift=%u", thread,
				i, thread_rsc_attribute->fmr_arr[i].fmr_props.max_pages,
				thread_rsc_attribute->fmr_arr[i].fmr_props.max_maps,
				thread_rsc_attribute->fmr_arr[i].fmr_props.page_shift));

		/* save the maximum of pages needed to be allocated later */
		if (thread_rsc_attribute->max_fmr_pages < thread_rsc_attribute->fmr_arr[i].fmr_props.max_pages)
			thread_rsc_attribute->max_fmr_pages = thread_rsc_attribute->fmr_arr[i].fmr_props.max_pages;

		thread_rsc_attribute->fmr_arr[i].num_of_remap = 0;
	}

	size = (thread_rsc_attribute->max_fmr_pages + 1) << used_page_shift;
	thread_rsc_attribute->buf_va = kmalloc(size, GFP_KERNEL);
	if (!thread_rsc_attribute->buf_va) {
		VL_MEM_ERR(("thread(%d) buffer allocation of %Zu bytes failed", thread, size));
		ret = -1;
		goto exit_thread_func;
	}

	/* create CQ: no completion handler or async event handler was defined for this CQ */
	thread_rsc_attribute->cq_hndl = ib_create_cq(res.device, NULL, NULL, NULL, REQ_CQE_NUM, 0);
	if (IS_ERR(thread_rsc_attribute->cq_hndl)) {
		VL_DATA_ERR(("thread(%d) ib_create_cq failed", thread));
		ret = -1;
		goto exit_thread_func;
	}

	/* create QP */
	memset(&thread_rsc_attribute->qp_init_attr, 0, sizeof(thread_rsc_attribute->qp_init_attr));

	thread_rsc_attribute->qp_init_attr.send_cq = thread_rsc_attribute->cq_hndl;
	thread_rsc_attribute->qp_init_attr.recv_cq = thread_rsc_attribute->cq_hndl;
	thread_rsc_attribute->qp_init_attr.cap.max_send_wr = 4;
	thread_rsc_attribute->qp_init_attr.cap.max_recv_wr = 4;
	thread_rsc_attribute->qp_init_attr.cap.max_send_sge = 1;
	thread_rsc_attribute->qp_init_attr.cap.max_recv_sge = 1;
	thread_rsc_attribute->qp_init_attr.sq_sig_type = IB_SIGNAL_ALL_WR;
	thread_rsc_attribute->qp_init_attr.qp_type = IB_QPT_RC;

	/* create the QP that used to move data between the 2 threads */
	thread_rsc_attribute->qp_hndl = ib_create_qp(thread_rsc_attribute->pd_hndl, &thread_rsc_attribute->qp_init_attr);
	if (IS_ERR(thread_rsc_attribute->qp_hndl)) {
		VL_DATA_ERR(("thread(%d) ib_create_qp failed", thread));
		ret = -1;
		goto exit_thread_func;
	}

	VL_DATA_TRACE(("thread(%d) my port = %d, my qpn = 0x%x, ", thread, my_port, thread_rsc_attribute->qp_hndl->qp_num));

	/* create the QP that used to check that the FMR were invalidated */
	thread_rsc_attribute->lb_qp_hndl = ib_create_qp(thread_rsc_attribute->pd_hndl, &thread_rsc_attribute->qp_init_attr);
	if (IS_ERR(thread_rsc_attribute->lb_qp_hndl)) {
		VL_DATA_ERR(("thread(%d) ib_create_qp of LB QP failed", thread));
		ret = -1;
		goto exit_thread_func;
	}
	VL_DATA_TRACE(("thread(%d) lb qpn = 0x%x", thread, thread_rsc_attribute->lb_qp_hndl->qp_num));

	/* sync before exchanging the QP numbers between the 2 threads */
	gdata[thread_pair_idx].qpns[my_idx_in_pair] = thread_rsc_attribute->qp_hndl->qp_num;


	gdata[thread_pair_idx].step[my_idx_in_pair]++;
	do {
		RELEASE_SEMAPHORE;
		ACQUIRE_SEMAPHORE;
	} while (gdata[thread_pair_idx].step[my_idx_in_pair] > gdata[thread_pair_idx].step[other_idx_in_pair]);

	/**************************************************************
	* move data between the 2 threads
	**************************************************************/
	for (j = 0; j < config.num_of_iter; j ++) {
		int fmr_idx = VL_random(&gdata[thread_pair_idx].rand[my_idx_in_pair], config.num_of_fmr);


		thread_rsc_attribute->num_of_pages = VL_range(&gdata[thread_pair_idx].rand[my_idx_in_pair], 1, thread_rsc_attribute->fmr_arr[fmr_idx].fmr_props.max_pages);

		VL_MISC_TRACE1(("thread(%d) executing iteration %u", thread, j));
		if (j % 1000 == 0)
			VL_MISC_TRACE(("thread(%d) executing iteration %u", thread, j));

		/* if the last completion was good the QPs state is RTS, so there isn't any need to modify the QPs state */
		if (thread_rsc_attribute->last_status != IB_WC_SUCCESS) {
			rc = modify_qp_state_to_RESET(thread, thread_rsc_attribute->qp_hndl, &thread_rsc_attribute->modify_qp_props);
			if (rc) {
				ret = -1;
				goto exit_thread_func;
			}

			rc = modify_qp_state_to_RTS(thread, thread_rsc_attribute->qp_hndl, my_port, thread_rsc_attribute->peers_port_props.lid, gdata[thread_pair_idx].qpns[peers_idx_in_pair], &thread_rsc_attribute->psn, &thread_rsc_attribute->modify_qp_props);
			if (rc) {
				ret = -1;
				goto exit_thread_func;
			}

			VL_MISC_TRACE1(("thread(%d) my_lid=%d, peer's lid=%d", thread, thread_rsc_attribute->my_port_props.lid, thread_rsc_attribute->peers_port_props.lid));
			VL_MISC_TRACE1(("thread(%d) peer's qpn = 0x%x, dst_qpn=0x%x, dst_lid=%d", 
					thread, gdata[thread_pair_idx].qpns[peers_idx_in_pair],
					gdata[thread_pair_idx].qpns[peers_idx_in_pair], thread_rsc_attribute->peers_port_props.lid));
		}

		/**************************************************************
		* (re)map the FMR in every iteration of the test
		**************************************************************/

		thread_rsc_attribute->map.size = thread_rsc_attribute->num_of_pages << thread_rsc_attribute->fmr_arr[fmr_idx].fmr_props.page_shift;
		thread_rsc_attribute->used_va = (void*)(unsigned long)PAGE_ALIGN((u64)(unsigned long)(thread_rsc_attribute->buf_va));
		thread_rsc_attribute->pa = (u64)ib_dma_map_single (res.device, thread_rsc_attribute->used_va, thread_rsc_attribute->map.size, DMA_BIDIRECTIONAL);
		rc = ib_dma_mapping_error (res.device, thread_rsc_attribute->pa);
		if (rc) {
			VL_MEM_ERR(("thread(%d) there was an ib_dma_mapping_error", thread));
			ret = -1;
			goto exit_thread_func;
		}

		thread_rsc_attribute->map.list_len = thread_rsc_attribute->num_of_pages;
		size = sizeof(u64) * thread_rsc_attribute->map.list_len;
		thread_rsc_attribute->map.page_list = vmalloc(size);
		if (!thread_rsc_attribute->map.page_list) {
			VL_MEM_ERR(("thread(%d) allocation of %Zu bytes for buffer failed", thread, size));
			ret = -1;
			goto exit_thread_func;
		}

		for (i = 0; i < thread_rsc_attribute->map.list_len; i++)
			thread_rsc_attribute->map.page_list[i] = thread_rsc_attribute->pa + (i << thread_rsc_attribute->fmr_arr[fmr_idx].fmr_props.page_shift);

		/* save the old keys as the previous keys (before getting new keys) */
		thread_rsc_attribute->fmr_arr[fmr_idx].prev_l_key = thread_rsc_attribute->fmr_arr[fmr_idx].l_key;
		thread_rsc_attribute->fmr_arr[fmr_idx].prev_r_key = thread_rsc_attribute->fmr_arr[fmr_idx].r_key;

		rc = ib_map_phys_fmr (thread_rsc_attribute->fmr_arr[fmr_idx].fmr_hndl,
					thread_rsc_attribute->map.page_list, 
					thread_rsc_attribute->map.list_len,
					(u64)(unsigned long)thread_rsc_attribute->used_va);
		if (rc) {
			VL_MEM_ERR(("thread(%d) ib_map_phys_fmr[%u] failed, rc = 0x%x", thread, fmr_idx, rc));
			ret = -1;
			goto exit_thread_func;
		}

		VL_MEM_TRACE1(("thread(%d): remapped FMR[%u] addr=0x%p, lkey=0x%x, rkey=0x%x",
				thread, fmr_idx, thread_rsc_attribute->used_va,
				thread_rsc_attribute->fmr_arr[fmr_idx].fmr_hndl->lkey,
				thread_rsc_attribute->fmr_arr[fmr_idx].fmr_hndl->rkey));

		/* save the new keys /.autodirect/swgwork/saeedm/regression/sw-tests/IB/kernel/libibverbs/fmr_test/fmr_test.c:55:*/
		thread_rsc_attribute->fmr_arr[fmr_idx].l_key = thread_rsc_attribute->fmr_arr[fmr_idx].fmr_hndl->lkey;
		thread_rsc_attribute->fmr_arr[fmr_idx].r_key = thread_rsc_attribute->fmr_arr[fmr_idx].fmr_hndl->rkey;

		vfree(thread_rsc_attribute->map.page_list);
		thread_rsc_attribute->map.page_list = NULL;

		/* increase the counter of the times that we did remaps to the fmr */
		thread_rsc_attribute->fmr_arr[fmr_idx].num_of_remap ++;

		VL_MISC_TRACE1(("thread(%d) lkey=0x%x, rkey=0x%x", thread, thread_rsc_attribute->fmr_arr[fmr_idx].l_key, thread_rsc_attribute->fmr_arr[fmr_idx].r_key));

		gdata[thread_pair_idx].rkeys[my_idx_in_pair] = thread_rsc_attribute->fmr_arr[fmr_idx].r_key;
		gdata[thread_pair_idx].mr_sizes[my_idx_in_pair] = thread_rsc_attribute->map.size;
		gdata[thread_pair_idx].acl[my_idx_in_pair] = thread_rsc_attribute->fmr_arr[fmr_idx].acl;
		gdata[thread_pair_idx].vas[my_idx_in_pair] = (u64)(unsigned long)thread_rsc_attribute->used_va;

		/* put 0 in the completed before posting any WR */
		gdata[thread_pair_idx].completed[my_idx_in_pair] = 0;

		/* the requestor decides which operation to execute and check what is the expected status */
		if (my_idx_in_pair == 0) {
			/* requestor */
			gdata[thread_pair_idx].opcode = VL_random(&gdata[thread_pair_idx].rand[my_idx_in_pair], 2) ? IB_WR_RDMA_WRITE : IB_WR_RDMA_READ;
		}

		/* sync before data movement between the 2 threads (and get the updated remote r_key, fmr size, status, opcode)*/
		gdata[thread_pair_idx].step[my_idx_in_pair]++;
		do {
			RELEASE_SEMAPHORE;
			ACQUIRE_SEMAPHORE;
		} while (gdata[thread_pair_idx].step[my_idx_in_pair] > gdata[thread_pair_idx].step[other_idx_in_pair]);

		thread_rsc_attribute->effective_mr_size = MIN(gdata[thread_pair_idx].mr_sizes[my_idx_in_pair], gdata[thread_pair_idx].mr_sizes[peers_idx_in_pair]);

		VL_MISC_TRACE1(("thread(%d) opcode = %s, my rkey=0x%x, my acl=0x%x, peer's rkey=0x%x, peer's acl=0x%x", thread, VL_ib_wr_opcode_str(gdata[thread_pair_idx].opcode),
				gdata[thread_pair_idx].rkeys[my_idx_in_pair], gdata[thread_pair_idx].acl[my_idx_in_pair],
				gdata[thread_pair_idx].rkeys[peers_idx_in_pair], gdata[thread_pair_idx].acl[peers_idx_in_pair]));

		/* put data in the buffer that is being send by the RDMA operation and initialize the other buffer */

		if (((my_idx_in_pair == 0) && (gdata[thread_pair_idx].opcode == IB_WR_RDMA_WRITE)) ||
			((my_idx_in_pair == 1) && (gdata[thread_pair_idx].opcode == IB_WR_RDMA_READ)))
		{ /* if this is the data that being sent - put validation data */
			/* initialize  the buffer */
			ib_dma_sync_single_for_cpu(res.device, thread_rsc_attribute->pa,
						thread_rsc_attribute->map.size, DMA_BIDIRECTIONAL);
			for (i = 0; i < thread_rsc_attribute->effective_mr_size; i++) {
				//*(((unsigned char *)((unsigned long)thread_rsc_attribute->used_va))+i) = (unsigned char)i;
				((unsigned char *)((unsigned long)thread_rsc_attribute->used_va))[i] = (unsigned char)i;
			}
			ib_dma_sync_single_for_device(res.device, thread_rsc_attribute->pa,
						thread_rsc_attribute->map.size, DMA_BIDIRECTIONAL);
		} else /* the data is being sent to this buffer */
			memset(thread_rsc_attribute->used_va, 0, thread_rsc_attribute->map.size);

		/* the requestor decides what is the expected status */
		if (my_idx_in_pair == 0) {
			/* requestor */
			enum ib_wc_status expected_status = IB_WC_SUCCESS;

			if (gdata[thread_pair_idx].opcode == IB_WR_RDMA_READ) {
				/* if there isn't any remote read permission */
				if (!(gdata[thread_pair_idx].acl[peers_idx_in_pair] & IB_ACCESS_REMOTE_READ)) {
					expected_status = IB_WC_REM_ACCESS_ERR;

					/* if there isn't any local write permission */
				} else if (!(gdata[thread_pair_idx].acl[my_idx_in_pair] & IB_ACCESS_LOCAL_WRITE)) {
					expected_status = IB_WC_LOC_PROT_ERR;
				}
			} else { /* the opcode if RDMA Write */
				/* if there isn't any remote write permission */
				if (!(gdata[thread_pair_idx].acl[peers_idx_in_pair] & IB_ACCESS_REMOTE_WRITE)) {
					expected_status = IB_WC_REM_ACCESS_ERR;
				}
			}

			gdata[thread_pair_idx].expected_status = expected_status;
		}

		/* sync after the data was initialized*/
		gdata[thread_pair_idx].step[my_idx_in_pair]++;
		if (my_idx_in_pair == 1) {
			/* responder */
			do {
				RELEASE_SEMAPHORE;
				ACQUIRE_SEMAPHORE;
			} while (gdata[thread_pair_idx].step[my_idx_in_pair] > gdata[thread_pair_idx].step[other_idx_in_pair]);
		} else {
			RELEASE_SEMAPHORE;
			ACQUIRE_SEMAPHORE;
		}

		/* requester */
		if (my_idx_in_pair == 0) {
			memset(&thread_rsc_attribute->sg_entry, 0, sizeof(thread_rsc_attribute->sg_entry));
			memset(&thread_rsc_attribute->sr_desc, 0, sizeof(thread_rsc_attribute->sr_desc));

			/* post send descriptor */
			thread_rsc_attribute->sg_entry.addr   = (unsigned long)thread_rsc_attribute->used_va;
			thread_rsc_attribute->sg_entry.length = thread_rsc_attribute->effective_mr_size;
			thread_rsc_attribute->sg_entry.lkey   = thread_rsc_attribute->fmr_arr[fmr_idx].l_key;

			thread_rsc_attribute->sr_desc.next                = NULL;
			thread_rsc_attribute->sr_desc.wr_id               = thread;
			thread_rsc_attribute->sr_desc.opcode              = gdata[thread_pair_idx].opcode;
			thread_rsc_attribute->sr_desc.send_flags          = IB_SEND_SIGNALED;
			thread_rsc_attribute->sr_desc.sg_list             = &thread_rsc_attribute->sg_entry;
			thread_rsc_attribute->sr_desc.num_sge             = 1;
			thread_rsc_attribute->sr_desc.wr.rdma.remote_addr = gdata[thread_pair_idx].vas[peers_idx_in_pair];
			thread_rsc_attribute->sr_desc.wr.rdma.rkey        = gdata[thread_pair_idx].rkeys[peers_idx_in_pair];


			rc = ib_post_send(thread_rsc_attribute->qp_hndl, &thread_rsc_attribute->sr_desc, &bad_send_wr);
			if (rc) {
				VL_DATA_ERR(("thread(%d) ib_post_send failed", thread));
				ret = -1;
				gdata[thread_pair_idx].completed[my_idx_in_pair] = 4;
				goto exit_thread_func;
			}
			VL_MISC_TRACE1(("thread(%d) posted to rkey=0x%x", thread, gdata[thread_pair_idx].rkeys[peers_idx_in_pair]));

			poll_start_time = jiffies;

			do {
				poll_total_time = jiffies - poll_start_time;

				rc = ib_poll_cq(thread_rsc_attribute->cq_hndl, 1, &thread_rsc_attribute->comp_desc);
				if (rc == 0) {
					if (jiffies_to_usecs(poll_total_time) < (MAX_COMP_TIMEOUT * 1000))
						yield();
					else 
						break;
				}
			} while (rc == 0);
			VL_MISC_TRACE1(("thread(%d) polled %d", thread, rc));

			/* If a completion wasn't read */
			if (rc <= 0) { 
				if (rc < 0)
					VL_DATA_ERR(("thread(%d) ib_poll_cq failed", thread));
				else if (!rc) /* If we stopped polling for completion because the timeout expired */
					VL_DATA_ERR(("thread(%d): A completion with status %s wasn't found in the CQ after %d msec", 
						thread, VL_ib_wc_status_str(gdata[thread_pair_idx].expected_status) , MAX_COMP_TIMEOUT));
			
				ret = -1;
				gdata[thread_pair_idx].completed[my_idx_in_pair] = 3;
				goto exit_thread_func;
			}

			if (thread_rsc_attribute->comp_desc.status != gdata[thread_pair_idx].expected_status) {
				VL_MISC_ERR(("thread(%d) local QP number=0x%x, opcode = %s, expected completion status: %s, actual status: %s (syndrome = 0x%x)", thread, thread_rsc_attribute->qp_hndl->qp_num,
						VL_ib_wr_opcode_str(thread_rsc_attribute->sr_desc.opcode), VL_ib_wc_status_str(gdata[thread_pair_idx].expected_status), VL_ib_wc_status_str(thread_rsc_attribute->comp_desc.status),
						thread_rsc_attribute->comp_desc.vendor_err));
				ret = -1;
				gdata[thread_pair_idx].completed[my_idx_in_pair] = 2;
				goto exit_thread_func;
			} else {
				VL_MISC_TRACE1(("thread(%d) successful completion with status %s", 
					thread, VL_ib_wc_status_str(thread_rsc_attribute->comp_desc.status)));
				gdata[thread_pair_idx].completed[my_idx_in_pair] = 1;
			}
		}

		/* check the data that was received */
		if (((my_idx_in_pair == 1) && (gdata[thread_pair_idx].opcode == IB_WR_RDMA_WRITE)) ||
			((my_idx_in_pair == 0) && (gdata[thread_pair_idx].opcode == IB_WR_RDMA_READ))) {

			/* if this is the responder */
			if (my_idx_in_pair == 1) {
				while (gdata[thread_pair_idx].completed[peers_idx_in_pair] == 0)
					yield();

				if (gdata[thread_pair_idx].completed[peers_idx_in_pair] != 1) {
					ret = -1;
					goto exit_thread_func;
				}
			}

			/* only upon a successfull completion validate the data */
			if (gdata[thread_pair_idx].expected_status == IB_WC_SUCCESS) {
				ib_dma_sync_single_for_cpu(res.device, thread_rsc_attribute->pa,
							thread_rsc_attribute->map.size, DMA_BIDIRECTIONAL);
				for (i = 0; i < thread_rsc_attribute->effective_mr_size; i++) {
					unsigned char actual =  *(((unsigned char *)((unsigned long)thread_rsc_attribute->used_va))+i);
					if (actual != (unsigned char )i ) {
						VL_MISC_ERR(("thread(%d) compare data failed. actual %d, expected %d\n", thread, actual, i));
						ret = -1;
						goto exit_thread_func;
					}
				}
				ib_dma_sync_single_for_device(res.device, thread_rsc_attribute->pa,
							thread_rsc_attribute->map.size, DMA_BIDIRECTIONAL);
				VL_MISC_TRACE1(("thread(%d) compare data success", thread));
			}
		}

		/* save the status from the last time (before it is being changed)*/
		thread_rsc_attribute->last_status = gdata[thread_pair_idx].expected_status;

		/* sync in the end of the test (before the FMR is being remapped */
		gdata[thread_pair_idx].step[my_idx_in_pair]++;
		do {
			RELEASE_SEMAPHORE;
			ACQUIRE_SEMAPHORE;
		} while (gdata[thread_pair_idx].step[my_idx_in_pair] > gdata[thread_pair_idx].step[other_idx_in_pair]);

		/* if there is a must to do unmap to remove all the outstanding remaps - do it */
		if (thread_rsc_attribute->fmr_arr[fmr_idx].num_of_remap == thread_rsc_attribute->fmr_arr[fmr_idx].fmr_props.max_maps) {

			num_of_inv_fmr = 0;
			INIT_LIST_HEAD(&thread_rsc_attribute->fmr_hndl_list);
			/* collect in random way some of the FMRs and do unmap to them */
			for (i = 0; i < config.num_of_fmr; i ++) {
				if ((VL_random(&gdata[thread_pair_idx].rand[my_idx_in_pair], 3) == 0) || (i == fmr_idx)) {
					
					list_add(&thread_rsc_attribute->fmr_arr[i].fmr_hndl->list, &thread_rsc_attribute->fmr_hndl_list);
					thread_rsc_attribute->fmr_arr[i].num_of_remap = 0;
					num_of_inv_fmr ++;
				}
			}

			rc = ib_unmap_fmr(&thread_rsc_attribute->fmr_hndl_list);
			if (rc) {
				VL_MEM_ERR(("thread(%d) ib_unmap_fmr failed", thread));
				ret = -1;
				goto exit_thread_func;
			}

			VL_MISC_TRACE1(("thread(%d) %u FMRs were unmapped", thread, num_of_inv_fmr));

			/* verify that all the FMRs that were unmapped cannot be accesses using the old keys */
			thread_rsc_attribute->validate_data.qp_hndl = thread_rsc_attribute->lb_qp_hndl;
			thread_rsc_attribute->validate_data.cq_hndl = thread_rsc_attribute->cq_hndl;
			thread_rsc_attribute->validate_data.thread = thread;
			thread_rsc_attribute->validate_data.port = my_port;
			thread_rsc_attribute->validate_data.dlid = thread_rsc_attribute->my_port_props.lid;
			thread_rsc_attribute->validate_data.dest_qp_num = thread_rsc_attribute->lb_qp_hndl->qp_num;
			thread_rsc_attribute->validate_data.my_idx_in_pair = my_idx_in_pair;

			for (i = 0; i < config.num_of_fmr; i ++) {
				/* if the FMR was unmapped */
				if (thread_rsc_attribute->fmr_arr[i].num_of_remap == 0) {
					VL_MISC_TRACE1(("thread(%d) verifying that FMR[%u] is invalidated", thread, i));

					/* validate that the previous keys are invalidated */
					rc = verify_invalidated_fmr(thread_pair_idx, my_idx_in_pair, peers_idx_in_pair, &thread_rsc_attribute->validate_data, thread_rsc_attribute->fmr_arr[i].prev_l_key, thread_rsc_attribute->fmr_arr[i].prev_r_key, 1 << thread_rsc_attribute->fmr_arr[i].fmr_props.page_shift, &thread_rsc_attribute->modify_qp_props, &thread_rsc_attribute->invalidate_fmr_props, &thread_rsc_attribute->lb_psn);
					if (rc) {
						ret = -1;
						goto exit_thread_func;
					}

					/* validate the last keys are invalidated (because we unmapped this FMR) */
					rc = verify_invalidated_fmr(thread_pair_idx, my_idx_in_pair, peers_idx_in_pair, &thread_rsc_attribute->validate_data, thread_rsc_attribute->fmr_arr[i].l_key, thread_rsc_attribute->fmr_arr[i].r_key, 1 << thread_rsc_attribute->fmr_arr[i].fmr_props.page_shift, &thread_rsc_attribute->modify_qp_props, &thread_rsc_attribute->invalidate_fmr_props, &thread_rsc_attribute->lb_psn);
					if (rc) {
						ret = -1;
						goto exit_thread_func;
					}
				}
			}
		}

		ib_dma_unmap_single (res.device, thread_rsc_attribute->pa, thread_rsc_attribute->map.size, DMA_BIDIRECTIONAL);
		thread_rsc_attribute->pa = 0;
	}

exit_thread_func:
	/**************************************************************
	* cleanup section
	**************************************************************/
	/* if the thread has failed */
	if (ret == -1) {
		/* mark the thread failed and release the semaphore to prevent a deadlock*/
		gdata[thread_pair_idx].completed[my_idx_in_pair] = -1;
		RELEASE_SEMAPHORE;
	}

	if (thread_rsc_attribute) {
		if (!IS_ERR(thread_rsc_attribute->lb_qp_hndl)) {
			rc = ib_destroy_qp(thread_rsc_attribute->lb_qp_hndl);
			if (rc) {
				VL_DATA_ERR(("thread(%d) ib_destroy_qp failed", thread));
				ret = -1;
			}
		}

		if (!IS_ERR(thread_rsc_attribute->qp_hndl)) {
			rc = ib_destroy_qp(thread_rsc_attribute->qp_hndl);
			if (rc) {
				VL_DATA_ERR(("thread(%d) ib_destroy_qp failed", thread));
				ret = -1;
			}
		}

		if (!IS_ERR(thread_rsc_attribute->cq_hndl)) {
			rc = ib_destroy_cq(thread_rsc_attribute->cq_hndl);
			if (rc) {
				VL_DATA_ERR(("thread(%d) ib_destroy_cq failed", thread));
				ret = -1;
			}
		}

		if (thread_rsc_attribute->pa)
			ib_dma_unmap_single (res.device, thread_rsc_attribute->pa, thread_rsc_attribute->map.size, DMA_BIDIRECTIONAL);

		if (thread_rsc_attribute->map.page_list)
			vfree(thread_rsc_attribute->map.page_list);

		if (thread_rsc_attribute->buf_va)
			kfree(thread_rsc_attribute->buf_va);

		/* collect all the valid FMR handles and unmap all of them at once */
		INIT_LIST_HEAD(&thread_rsc_attribute->fmr_hndl_list);
		for (i = 0; i < config.num_of_fmr; i ++) {
			if (!IS_ERR(thread_rsc_attribute->fmr_arr[i].fmr_hndl))
					list_add(&thread_rsc_attribute->fmr_arr[i].fmr_hndl->list, &thread_rsc_attribute->fmr_hndl_list);
		}
			
		rc = ib_unmap_fmr(&thread_rsc_attribute->fmr_hndl_list);
		if (rc) {
			VL_MEM_ERR(("thread(%d) ib_unmap_fmr failed", thread));
			ret = -1;
		}

		if (thread_rsc_attribute->fmr_arr) {
			for (i = 0; i < config.num_of_fmr; i++) {
				if (!IS_ERR(thread_rsc_attribute->fmr_arr[i].fmr_hndl)) {
					rc = ib_dealloc_fmr(thread_rsc_attribute->fmr_arr[i].fmr_hndl);
					if (rc) {
						VL_MEM_ERR(("thread(%d) ib_dealloc_fmr[%u] failed", thread, i));
						ret = -1;
					}
				}
			}

			vfree(thread_rsc_attribute->fmr_arr);
		}

		if (!IS_ERR(thread_rsc_attribute->pd_hndl)) {
			rc = ib_dealloc_pd(thread_rsc_attribute->pd_hndl);
			if (rc) {
				VL_HCA_ERR(("thread(%d) ib_dealloc_pd failed", thread));
				ret = -1;
			}
		}
		vfree(thread_rsc_attribute);
	}

	VL_MISC_TRACE(("thread %d was ended with status %d", thread, ret));

	return ret;
}

/***************************************
* Function: test_driver_write
***************************************/
ssize_t run_fmr_test(IN struct config_t *config_p)
{
	int rval = -EINVAL;
	int rc;
	int registered = 0;
	int created_pairs = 0;
	int i, j;
	int size;
	memcpy(&config,config_p,sizeof(config));

	VL_MISC_TRACE(("******************[fmr_test]******************"));
	VL_MISC_TRACE(("* HCA id                      : %s", config.dev_name));
	VL_MISC_TRACE(("* IB port                     : %u", config.ib_port));
	VL_MISC_TRACE(("* Random seed                 : %lu", config.seed));
	VL_MISC_TRACE(("* Number of iterations        : %u", config.num_of_iter));
	VL_MISC_TRACE(("* Number of FMRs (per thread) : %u", config.num_of_fmr));
	VL_MISC_TRACE(("* Use bad flow                : %s", ((config.bad_flow) ? "yes" : "no")));
	VL_MISC_TRACE(("* Number of thread pairs      : %u", config.num_thread_pairs));
	VL_MISC_TRACE(("* Trace level                 : 0x%x", config.trace_level));
	VL_MISC_TRACE(("***************************************"));

	VL_set_verbosity_level(config.trace_level);

	/* register the module to the IB driver */
	exit_val = -1;

	rc = ib_register_client(&test_client);
	if (rc) {
		VL_MISC_ERR(("%s couldn't register IB client", VL_driver_generic_name));
		rval = -ENODEV;
		goto cleanup;
	}
	registered = 1;
	if (exit_val) {
		VL_MISC_ERR(("%s failed because exit_val is not zero", VL_driver_generic_name));
		rval = -ENODEV;
		goto cleanup;
	}

	/* prepare the data for all of the threads */
	size = sizeof(struct thread_global_data_t) * config.num_thread_pairs;
	gdata = vmalloc(size);
	if (!gdata) {
		VL_MEM_ERR(("%s failed allocating memory for threads objects", VL_driver_generic_name));
		rval = -ENOMEM;
		goto cleanup;
	}

	/* init all of the threads resources */
	for (created_pairs = 0; created_pairs < config.num_thread_pairs; created_pairs ++) {
		for (i = 0; i < 2; i ++) {
			sema_init(&gdata[created_pairs].sems[i], 0);
			gdata[created_pairs].completed[i] = 0;
			gdata[created_pairs].step[i] = 0;
		}

		gdata[created_pairs].expected_status = IB_WC_GENERAL_ERR;
	}

	for (created_pairs = 0; created_pairs < config.num_thread_pairs; created_pairs ++) {
		/* start the two threads */
		for (i = 0; i < 2; i ++) {
			rc = VL_thread_start(&gdata[created_pairs].threads[i], 0, (void*)thread_func, (void *)(unsigned long)(2*created_pairs+i));
			if (rc != VL_OK) {
				VL_MISC_ERR(("%s failed to create thread[%d] in pair %d", VL_driver_generic_name, i, created_pairs));
				goto cleanup;
			}
		}
	}


	rval = 0;
cleanup:

	for (j = 0; j < created_pairs; j ++) {
		int exit_code;
		/* wait for the thread completion  */
		for (i = 0; i < 2; i ++) {
			rc = VL_thread_wait_for_term(&gdata[j].threads[i], &exit_code);
			if ((rc != VL_OK) || (exit_code)) {
				VL_MISC_ERR(("%s error in thread[%d] in pair %d", VL_driver_generic_name, i, j));
				rval = -EINVAL;
			}
		}
	}

	if (gdata)
		vfree(gdata);
	
	if (registered)
		ib_unregister_client(&test_client);


	VL_MISC_TRACE1(("%s exit write", VL_driver_generic_name));
	VL_print_test_status(rval);

	return rval;
}

/***************************************
* Function: test_add_one
***************************************/
static void test_add_one(
	IN	struct ib_device *device)
{

	VL_MISC_TRACE1(("%s enter query_add_one\n", VL_driver_generic_name));

	if (strcmp(device->name, config.dev_name) != 0) {
		VL_MISC_TRACE(("%s test_add_one device name %s doesn't match.", VL_driver_generic_name, device->name));
		goto exit;
	}
	VL_HCA_TRACE(("The IB device %s was found", device->name));

	res.device = device;

	exit_val = 0;
exit:

	VL_MISC_TRACE1(("%s exit query_add_one", VL_driver_generic_name));
}

/***************************************
* Function: test_remove_one
***************************************/
static void test_remove_one(
	IN	struct ib_device *device)
{
	VL_MISC_TRACE1(("%s enter query_remove_one", VL_driver_generic_name));
	schedule();

	if ((config.dev_name) && (strcmp(device->name, config.dev_name)) != 0) {
		VL_MISC_ERR(("%s test_remove_one device name %s doesn't match.", VL_driver_generic_name, device->name));
		exit_val = -1;
		return;
	}

	VL_MISC_TRACE1(("%s exit query_remove_one", VL_driver_generic_name));
}

