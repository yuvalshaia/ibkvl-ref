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
 * $Id: basic_module.c 2699 2007-01-15 08:12:46Z dotanb $ 
 * 
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/list.h>		/* for list handling */
#include <linux/vmalloc.h>	/* for vmalloc */
#include <linux/sched.h>	/* for yield */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
        #include <asm/semaphore.h>      /* for semaphore */
#else
        #include <linux/semaphore.h>
#endif
#include <asm/uaccess.h>	/* for copy_from_user */

#include <vl_ib_verbs.h>
#include <vl.h>
#include <vl_gen2k_str.h>

#include <kvl.h>

#include "basic_module.h"
#include "types.h"
#include "check_operations.h"

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
static const char *VL_driver_generic_name = "atomic_test:";
static volatile int exit_val = -1;
static struct test_resources_t test_resources;
static struct dev_resource_t dev_res;
static struct remote_resources_t rem_resources;
static struct config_t config = {0};
static int registered = 0;


struct ib_client test_client = {
	.name   = "atomic_test",
	.add    = test_add_one,
	.remove = test_remove_one
};


/***************************************
* Function: init_resources
****************************************
* Description: init test resources
***************************************/
static void init_resources(void)
{
	/* init test_resources */
	test_resources.pd_hndl		= ERR_PTR(-EINVAL);
	test_resources.mr_hndl		= ERR_PTR(-EINVAL);
	test_resources.qp_hndl_arr	= NULL;
	test_resources.cq_hndl		= ERR_PTR(-EINVAL);
	test_resources.buf		= NULL;
	test_resources.qp_num_arr	= NULL;

	memset(&test_resources.atomic_data_addr, 0, sizeof(test_resources.atomic_data_addr));
}

/***************************************
* Function: destroy_resources
****************************************
* Description: destroy test resources
***************************************/
static int destroy_resources(void)
{
	int ret = 0;
	int rc, i;

	/* clean resources on failure */
	if (test_resources.qp_hndl_arr) {
		for (i = 0; i < config.num_of_qps; i++) {
			if (!IS_ERR(test_resources.qp_hndl_arr[i])) {
				rc = ib_destroy_qp(test_resources.qp_hndl_arr[i]);
				if (rc) {
					VL_DATA_ERR(("ib_destroy_qp[%d] failed", i));
					ret = -1;
				}
			}
		}
		vfree(test_resources.qp_hndl_arr);
	}

	if (test_resources.qp_num_arr)
		kfree(test_resources.qp_num_arr);

	if (!IS_ERR(test_resources.cq_hndl)) {
		rc = ib_destroy_cq(test_resources.cq_hndl);
		if (rc) {
			VL_DATA_ERR(("ib_destroy_cq failed"));
			ret = -1;
		}
	}

	if (!IS_ERR(test_resources.mr_hndl) && test_resources.mr_hndl) {
		rc = ib_dereg_mr(test_resources.mr_hndl);
		if (rc) {
			VL_DATA_ERR(("ib_dereg_mr failed"));
			ret = -1;
		}
	}

	if (!IS_ERR(test_resources.pd_hndl)) {
		rc = ib_dealloc_pd(test_resources.pd_hndl);
		if (rc) {
			VL_HCA_ERR(("ib_dealloc_pd failed"));
			ret = -1;
		}
	}

	if (test_resources.atomic_data_addr.dma)
		ib_dma_unmap_single(dev_res.device, test_resources.atomic_data_addr.dma, 8, DMA_BIDIRECTIONAL);

	if (test_resources.buf)
		kfree(test_resources.buf);
	
	VL_MISC_TRACE(("Destroy resources - %s", (ret != 0) ? "FAILED": "PASSED"));
	return ret;
}

/***************************************
* Function: create_resources
****************************************
* Description: create test resources
***************************************/
static int create_resources(void)
{
	int ret = 0;
	size_t size;
	int rc, i;
	int cqe_num;


	/* allocate the QPs array and invalidate all the QPs */
	size = sizeof(struct ib_qp*) * config.num_of_qps;
	test_resources.qp_hndl_arr = vmalloc(size);
	if (!test_resources.qp_hndl_arr) {
		VL_MEM_ERR(("Allocation of %Zu bytes for QPs handls buffer failed", size));
		ret = -1;
		goto exit_create_resources;
	}

	for (i = 0; i < config.num_of_qps; i++)
		test_resources.qp_hndl_arr[i] =  ERR_PTR(-EINVAL);

	/* query the HCA attributes */
	rc = ib_query_device(dev_res.device, &test_resources.hca_cap);
	if (rc) {
		VL_HCA_ERR(("ib_query_device failed"));
		ret = -1;
		goto exit_create_resources;
	}

	/* query the port props */
	rc = ib_query_port(dev_res.device, config.ib_port, &test_resources.my_port_props);
	if (rc) {
		VL_HCA_ERR(("ib_query_port for port %u failed", config.ib_port));
		ret = -1;
		goto exit_create_resources;
	}

	if (test_resources.my_port_props.state != IB_PORT_ACTIVE) {
		VL_HCA_ERR(("Port %u state is not active: %s", config.ib_port,
			    VL_ib_port_state_str(test_resources.my_port_props.state)));
		ret = -1;
		goto exit_create_resources;
	}

	/* allocate PD */
	test_resources.pd_hndl = ib_alloc_pd(dev_res.device);
	if (IS_ERR(test_resources.pd_hndl)) {
		VL_HCA_ERR(("ib_alloc_pd failed"));
		ret = -1;
		goto exit_create_resources;
	}

	/* create MR */
	/* decide what will be the MR size */
	size = 8 * (config.num_of_qps * config.num_of_wrs) + 8 + 8; /* atomic operation send 8 bytes */

	/* make sure that the MR size will be atleast 15 (8 bytes + 7 bytes for alignment error) */
	if (size < 15)
		size = 15;

	test_resources.buf = kmalloc(size, GFP_KERNEL);
	if (!test_resources.buf) {
		VL_MEM_ERR(("Could not allocate %Zu bytes for memory buffer", size));
		ret = -1;
		goto exit_create_resources;
	}

	/* make sure that the atomic_data address is 8 byte aligned */
	test_resources.atomic_data_addr.physical = (void *)VL_ALIGN_UP((unsigned long)test_resources.buf, 3);
	VL_MEM_TRACE(("Atomic (shared) data address (physical) is %p", test_resources.atomic_data_addr.physical));

	if (config.is_daemon) {
		/* write inital value to address */
		*(uint64_t *)test_resources.atomic_data_addr.physical = config.seed;
		VL_MISC_TRACE(("Initial Value : "U64H_FMT, *(uint64_t *)test_resources.atomic_data_addr.physical));
	}

	test_resources.atomic_data_addr.dma = ib_dma_map_single(dev_res.device, (void *)test_resources.atomic_data_addr.physical, 8, DMA_BIDIRECTIONAL);
	if (ib_dma_mapping_error(dev_res.device, test_resources.atomic_data_addr.dma)) {
		VL_MEM_ERR(("dma_map_single failed"));
		ret = -1;
		goto exit_create_resources;
	}
	VL_MEM_TRACE(("Atomic (shared) data address (dma) is %p", (void *)test_resources.atomic_data_addr.dma));

	test_resources.mr_hndl = ib_get_dma_mr(test_resources.pd_hndl, IB_ACCESS_LOCAL_WRITE | IB_ACCESS_REMOTE_ATOMIC);
	if (IS_ERR(test_resources.mr_hndl)) {
		VL_MEM_ERR(("ib_get_dma_mr failed"));
		ret = -1;
		goto exit_create_resources;
	}

	if (config.is_daemon)
		/* the daemon won't use the CQ */
		cqe_num = 2;
	else
		cqe_num = config.num_of_qps * VL_MIN(test_resources.hca_cap.max_qp_rd_atom,
						  test_resources.hca_cap.max_qp_init_rd_atom);

	/* create CQ  */
	test_resources.cq_hndl = ib_create_cq(dev_res.device, NULL, NULL, NULL, cqe_num, 0);
	if (IS_ERR(test_resources.cq_hndl)) {
		VL_DATA_ERR(("ib_create_cq failed"));
		ret = -1;
		goto exit_create_resources;
	}

	/* create QPs */
	test_resources.qp_num_arr = (uint32_t*)kmalloc(sizeof(uint32_t) * config.num_of_qps, GFP_KERNEL);
	if (!test_resources.qp_num_arr){
		VL_MISC_ERR(("Failed to malloc test_resources.qp_num_arr struct"));
		ret = -1;
		goto exit_create_resources;
	}

	memset(&test_resources.qp_init_attr, 0, sizeof(test_resources.qp_init_attr));

	test_resources.qp_init_attr.send_cq = test_resources.cq_hndl;
	test_resources.qp_init_attr.recv_cq = test_resources.cq_hndl;
	test_resources.qp_init_attr.cap.max_send_wr = VL_MIN(test_resources.hca_cap.max_qp_init_rd_atom, test_resources.hca_cap.max_qp_rd_atom);
	test_resources.qp_init_attr.cap.max_recv_wr = 0;
	test_resources.qp_init_attr.cap.max_send_sge = 1;
	test_resources.qp_init_attr.cap.max_recv_sge = 0;
	test_resources.qp_init_attr.sq_sig_type = IB_SIGNAL_ALL_WR;
	test_resources.qp_init_attr.qp_type = IB_QPT_RC;

	for (i = 0; i < config.num_of_qps; i++) {

		test_resources.qp_hndl_arr[i] = ib_create_qp(test_resources.pd_hndl, &test_resources.qp_init_attr);
		if (IS_ERR(test_resources.qp_hndl_arr[i])) {
			VL_DATA_ERR(("ib_create_qp of QP[%d] failed", i));
			ret = -1;
			goto exit_create_resources;
		}
		test_resources.qp_num_arr[i] = test_resources.qp_hndl_arr[i]->qp_num;
	}

exit_create_resources:

	VL_MISC_TRACE(("Create resources - %s", (ret != 0) ? "FAILED": "PASSED"));

	return ret;
}


/***************************************
* Function: send_info2user
****************************************
* Description:  send qps info to user
***************************************/
static int send_info2user(struct u2k_cmd_t *cmd_p)
{
	int rc;

	rem_resources.remote_addr    = test_resources.atomic_data_addr.dma;
	rem_resources.rkey           = test_resources.mr_hndl->rkey;
	rem_resources.lid            = test_resources.my_port_props.lid;
	rem_resources.max_qp_rd_atom = test_resources.hca_cap.max_qp_rd_atom;

	rc = copy_to_user(rem_resources.qp_num_arr,  test_resources.qp_num_arr, sizeof(uint32_t) * config.num_of_qps);
	if (rc)
		goto clean_up;

	rc = copy_to_user(cmd_p->remote_resources, &rem_resources, sizeof(struct remote_resources_t));

clean_up:


	VL_MISC_TRACE(("Send QPS info to user - %s", (rc != 0) ? "FAILED": "PASSED"));
	return rc;
}


/***************************************
* Function: fill_qps
****************************************
* Description: fill qps 
***************************************/
static int fill_qps(void)
{
	struct ib_qp_attr	attr;
	int			i;
	int			flags;
	int			rc = 0;

	for (i = 0; i < config.num_of_qps; i++) {
		/* RESET -> INIT */
		memset(&attr, 0, sizeof(struct ib_qp_attr));

		attr.qp_state = IB_QPS_INIT;
		attr.port_num = config.ib_port;
		attr.pkey_index = 0;
		attr.qp_access_flags = IB_ACCESS_REMOTE_ATOMIC;

		flags = IB_QP_STATE | IB_QP_PKEY_INDEX | IB_QP_PORT | IB_QP_ACCESS_FLAGS;

		rc = ib_modify_qp(test_resources.qp_hndl_arr[i], &attr, flags);
		if (rc) {
			VL_DATA_ERR(("failed to modify QP[%d] state to INIT", i));
			return rc;
		}

		/* INIT -> RTR */
		memset(&attr, 0, sizeof(attr));

		attr.qp_state = IB_QPS_RTR;
		attr.path_mtu = IB_MTU_256;
		attr.dest_qp_num = rem_resources.qp_num_arr[i];
		attr.rq_psn = 0;
		attr.max_dest_rd_atomic = test_resources.hca_cap.max_qp_rd_atom;
		attr.min_rnr_timer = 0;
		attr.ah_attr.ah_flags = 0;
		attr.ah_attr.static_rate = 0;
		attr.ah_attr.dlid = rem_resources.lid;
		attr.ah_attr.sl = 0;
		attr.ah_attr.src_path_bits = 0;
		attr.ah_attr.port_num = config.ib_port;

		flags = IB_QP_STATE | IB_QP_AV | IB_QP_PATH_MTU | IB_QP_DEST_QPN |
			IB_QP_RQ_PSN | IB_QP_MAX_DEST_RD_ATOMIC | IB_QP_MIN_RNR_TIMER;

		rc = ib_modify_qp(test_resources.qp_hndl_arr[i], &attr, flags);
		if (rc) {
			VL_DATA_ERR(("failed to modify QP[%d] state to RTR", i));
			return rc;
		}

		/* RTR -> RTS */
		memset(&attr, 0, sizeof(attr));

		attr.qp_state = IB_QPS_RTS;
		attr.timeout = 0x12;
		attr.retry_cnt = 6;
		attr.rnr_retry = 0;
		attr.sq_psn = 0;
		attr.max_rd_atomic = VL_MIN(test_resources.hca_cap.max_qp_init_rd_atom, rem_resources.max_qp_rd_atom);

		flags = IB_QP_STATE | IB_QP_TIMEOUT | IB_QP_RETRY_CNT |
			IB_QP_RNR_RETRY | IB_QP_SQ_PSN | IB_QP_MAX_QP_RD_ATOMIC;

		rc = ib_modify_qp(test_resources.qp_hndl_arr[i], &attr, flags);
		if (rc) {
			VL_DATA_ERR(("failed to modify QP[%d] state to RTS", i));
			return rc;
		}
		VL_DATA_TRACE1(("QP[%d] state was changed to RTS", i));
	}
	VL_DATA_TRACE(("All QPs state were changed to RTS"));

	return rc;
}

/***************************************
* Function: get_expected_fetch_and_add
****************************************
* Description: get_expected_fetch_and_add
***************************************/
static uint64_t get_expected_fetch_and_add(
	IN			uint64_t init_val,
	IN			uint64_t add_value,
	IN			uint64_t mask_fa)
{
	uint64_t expected_val = init_val;
	int i;

	if (mask_fa == MAX64)
		return (init_val + (config.num_of_iter * config.num_of_qps * config.num_of_wrs) * add_value);

	for (i = 0; i < (config.num_of_iter * config.num_of_qps * config.num_of_wrs) ; i++) {
		expected_val = calc_msk_fetch_and_add(add_value ,mask_fa ,expected_val);
		// printk("Expected val after iter %d = "U64H_FMT"\n",i,expected_val);
	}
	VL_MISC_TRACE(("total number of iterations: %d", config.num_of_iter * config.num_of_qps * config.num_of_wrs));

	return expected_val;
}

/***************************************
* Function: fetch_and_add
****************************************
* Description: fetch_and_add 
***************************************/
static int fetch_and_add(
	IN			struct VL_random_t  *rand)
{
	struct ib_send_wr sr_desc;
	struct ib_sge sg_entry;
	struct ib_wc comp_desc;
	uint64_t init_val, value_to_add, expected_val, mask_fa = MAX64;
	struct ib_send_wr *bad_wr;
	unsigned long offset = 0, last_offset = 0;
	unsigned int i, j, k;
	unsigned int posted_num_of_wr;
	unsigned int completions_to_poll;
	unsigned int max_atomic_oust = VL_MIN(test_resources.hca_cap.max_qp_init_rd_atom, test_resources.hca_cap.max_qp_rd_atom);
	int ret_val = 1;
	int rc = 0;
	uint64_t tmp = 0;

	init_val        = (uint64_t)config.seed;
	value_to_add    = VL_random64(rand, MAX64);

	VL_MISC_TRACE(("Fetch & Add test was started"));

	VL_DATA_TRACE1(("Add value          : "U64H_FMT, value_to_add));
	VL_DATA_TRACE1(("Remote address     : "U64H_FMT, rem_resources.remote_addr));
	VL_DATA_TRACE1(("Remote key         : 0x%x", rem_resources.rkey));
	VL_DATA_TRACE1(("Local key          : 0x%x", test_resources.mr_hndl->rkey));

	/* do the test */
	memset(&sr_desc, 0, sizeof(sr_desc));
	memset(&sg_entry, 0, sizeof(sg_entry));

	sg_entry.length = 8;
	sg_entry.lkey   = test_resources.mr_hndl->rkey;

	sr_desc.next                  = NULL;
	sr_desc.sg_list               = &sg_entry;
	sr_desc.num_sge               = 1;
	sr_desc.wr.atomic.compare_add = value_to_add;
	sr_desc.wr.atomic.remote_addr = rem_resources.remote_addr;
	sr_desc.wr.atomic.rkey        = rem_resources.rkey;

	if (config.test_mode == F_AND_A)
		sr_desc.opcode                  = IB_WR_ATOMIC_FETCH_AND_ADD;
	else { /* config.test_mode== MF_AND_A */
		VL_MISC_TRACE(("Running masked Fetch & Add test"));
		mask_fa                         = VL_random64(rand, MAX64);
		VL_DATA_TRACE1(("fetch and add mask : "U64H_FMT, mask_fa));

		sr_desc.opcode               	   = IB_WR_MASKED_ATOMIC_FETCH_AND_ADD;
		sr_desc.wr.atomic.compare_add_mask = mask_fa;
	}


	for (i = 0; i < config.num_of_iter; i++) {
		posted_num_of_wr = 0;
		do {
			unsigned int wr_to_post;
			offset = 0;

			if ((config.num_of_wrs - posted_num_of_wr) > max_atomic_oust)
				wr_to_post = max_atomic_oust;
			else
				wr_to_post = config.num_of_wrs - posted_num_of_wr;

			/* post the SR */
			for (j = 0; j < config.num_of_qps; j++) {
				for (k = 0; k < wr_to_post; k++) {
					sg_entry.addr = (uint64_t)test_resources.atomic_data_addr.dma + offset;
					sr_desc.wr_id = sg_entry.addr;
					rc = ib_post_send(test_resources.qp_hndl_arr[j], &sr_desc, &bad_wr);
					if (rc) {
						VL_DATA_ERR(("failed to post SR[%u] to QP[%u]", k, j));
						return 1;
					}
					last_offset = offset;
					offset += 8;
				}
			}

			posted_num_of_wr += wr_to_post;

			completions_to_poll = config.num_of_qps * wr_to_post;

			/* collect all the completions */
			for (j = 0; j < completions_to_poll; j ++) {
				tmp = 0;
				do {
					rc = ib_poll_cq(test_resources.cq_hndl, 1, &comp_desc);
					if (rc == 0) {
						yield();
						if (tmp++ == 50000000) return 1;
					}
				} while (rc == 0);

				/* check that we don't have an error */
				if (rc < 0) {
					VL_DATA_ERR(("ib_poll_cq of completion[%u] failed", i));
					return 1;
				}

				if (comp_desc.status != IB_WC_SUCCESS) {
					VL_DATA_ERR(("Bad completion[%u] status: %s, syndrome: 0x%x\n",
						     j, VL_ib_wc_status_str(comp_desc.status), comp_desc.vendor_err));
					return 1;
				}
			}
		} while (posted_num_of_wr < config.num_of_wrs);
	}

	/* send one more time to get the last value from the remote address
	* send it after getting all other completions to prevent race wetween the WR's */
	last_offset = offset;
	sg_entry.addr = (uint64_t)test_resources.atomic_data_addr.dma + offset;
	sr_desc.wr_id = sg_entry.addr;

	/* post the SR */
	rc = ib_post_send(test_resources.qp_hndl_arr[0], &sr_desc, &bad_wr);
	if (rc) {
		VL_DATA_ERR(("failed to post last SR"));
		return 1;
	}

	/* collect all the completion */
	tmp = 0;
	do {
		rc = ib_poll_cq(test_resources.cq_hndl, 1, &comp_desc);
		if (rc == 0) {
			yield();
			if (tmp++ == 50000000) return 1;
		}
	} while (rc == 0);

	/* check that we don't have an error */
	if (rc < 0) {
		VL_DATA_ERR(("ib_poll_cq of last completion failed"));
		return 1;
	}

	if (comp_desc.status != IB_WC_SUCCESS) {
		VL_DATA_ERR(("Bad last completion status: %s, syndrome: 0x%x\n",
			     VL_ib_wc_status_str(comp_desc.status), comp_desc.vendor_err));
		return 1;
	}

	/* check that we got the expected value */
	expected_val = get_expected_fetch_and_add(init_val, value_to_add, mask_fa);
	ret_val = (expected_val != *(uint64_t *)(test_resources.atomic_data_addr.physical + last_offset)); 
	if (ret_val) {
		VL_DATA_ERR(("Fetch & Add FAILED:"));
		VL_DATA_ERR(("  Initial value : "U64H_FMT, init_val));
		VL_DATA_ERR(("  Add value     : "U64H_FMT, value_to_add));
		VL_DATA_ERR(("  Expected value: "U64H_FMT, expected_val));
		VL_DATA_ERR(("  Actual value  : "U64H_FMT, *(uint64_t *)(test_resources.atomic_data_addr.physical + last_offset)));
	} 

	VL_MISC_TRACE(("%sFetch & Add test has %s", (config.test_mode == F_AND_A)? "": "Masked ", (ret_val) ? "failed" : "passed"));
	return ret_val;
}

/***************************************
* Function: my_rand
****************************************
* Description: my_rand
***************************************/
static uint64_t my_rand(
	IN			struct VL_random_t  *rand)
{
	int rand_val = VL_range(rand, 1, 100);

	if (rand_val < 3) 
		return 0x0;
	if (rand_val > 97)
		return MAX64;

	return VL_random64(rand, MAX64); 
}

/***************************************
* Function: calc_near_comp_mask
****************************************
* Description: calc_near_comp_mask
***************************************/
static uint64_t calc_near_comp_mask(
	IN			struct VL_random_t  *rand,
	IN			uint64_t mask_compare)
{
	uint64_t new_mask = 0;
	uint64_t bit_check;
	int i;

	/* in 10% from the cases return the original mask */
	if (VL_range(rand, 1, 10) == 5) return mask_compare;

	for (i = 0; i < ADD_SIZE; i++) {
		bit_check = 1 << i;
		if (bit_check & mask_compare) {
			/* for each set bit in orig mask, set in in new mask in 95% of the times */
			if (VL_range(rand, 1, 100) < 95)
				new_mask |= bit_check;
		}
	}

	return new_mask;
}

/***************************************
* Function: comp_and_swap
****************************************
* Description: comp_and_swap 
***************************************/
static int comp_and_swap(
	IN			struct VL_random_t  *rand)
{
	struct ib_send_wr sr_desc;
	struct ib_sge sg_entry;
	struct ib_wc comp_desc;
	struct ib_send_wr *bad_wr;
	uint64_t returned_value, comp_val, swap_val, last_val;
	uint64_t mask_cs_compare = MAX64, mask_cs_swap = MAX64;
	uint64_t mask_cs_compare2, expected_val;
	unsigned int i, j, k;
	int num_of_success = 0;
	int rc = 0;
	uint64_t tmp = 0;

	last_val        = (uint64_t)config.seed;

	VL_MISC_TRACE(("Comp & Swap test was started"));

	VL_DATA_TRACE1(("Remote address     : "U64H_FMT, rem_resources.remote_addr));
	VL_DATA_TRACE1(("Remote key         : 0x%x", rem_resources.rkey));

	/* do the test */
	memset(&sr_desc, 0, sizeof(sr_desc));
	memset(&sg_entry, 0, sizeof(sg_entry));

	sg_entry.length = 8;
	sg_entry.lkey   = test_resources.mr_hndl->rkey;

	sr_desc.next                  = NULL;
	sr_desc.sg_list               = &sg_entry;
	sr_desc.num_sge               = 1;
	sr_desc.wr.atomic.remote_addr = rem_resources.remote_addr;
	sr_desc.wr.atomic.rkey        = rem_resources.rkey;

	if (config.test_mode == C_AND_S)
		sr_desc.opcode                  = IB_WR_ATOMIC_CMP_AND_SWP;
	else { /* config.test_mode == MC_AND_S */
		VL_MISC_TRACE1(("Running masked Comp & Swap"));
		sr_desc.opcode                  = IB_WR_MASKED_ATOMIC_CMP_AND_SWP;
	}

	for (i = 0; i < config.num_of_iter; i++) {
		VL_DATA_TRACE1(("Last known value in beginning of iteration : "U64H_FMT, last_val));
		for (j = 0; j < config.num_of_qps; j++) {
			for (k = 0; k < config.num_of_wrs; k++) {
				if (config.test_mode == MC_AND_S) {
					mask_cs_compare = my_rand(rand);
					mask_cs_swap    = my_rand(rand);
					sr_desc.wr.atomic.swap_mask        = mask_cs_swap;
					sr_desc.wr.atomic.compare_add_mask = mask_cs_compare;
				}
				swap_val = VL_random64(rand, MAX64);

				/* calculate new near  compare mask for getting new comp_val*/
				mask_cs_compare2 = calc_near_comp_mask(rand, mask_cs_compare);

				comp_val = (VL_random64(rand, MAX64) & (~mask_cs_compare2)) | (last_val & mask_cs_compare2);
				sr_desc.wr.atomic.compare_add   = comp_val;
				sr_desc.wr.atomic.swap          = swap_val;
				sg_entry.addr = (uint64_t)test_resources.atomic_data_addr.dma;
				sr_desc.wr_id = sg_entry.addr;

				/* post  SR */
				rc = ib_post_send(test_resources.qp_hndl_arr[j], &sr_desc, &bad_wr);
				if (rc) {
					VL_DATA_ERR(("failed to post SR[%u] to QP[%u]", k, j));
					return 1;
				}
				tmp = 0;
				/* poll cq*/                
				do {
					rc = ib_poll_cq(test_resources.cq_hndl, 1, &comp_desc);
					if (rc == 0){
					    yield();
						if (tmp++ == 50000000) {
							VL_MISC_ERR(("No completion found in CQ after %d iterations", 50000000)); 
							return 1;
						}
					}
				} while (rc == 0);

				/* check that we don't have an error */
				if (rc < 0) {
					VL_DATA_ERR(("ib_poll_cq of completion[%u] failed", k));
					return 1;
				}

				if (comp_desc.status != IB_WC_SUCCESS) {
					VL_DATA_ERR(("Bad completion[%u] status: %s, syndrome: 0x%x\n",
						     k, VL_ib_wc_status_str(comp_desc.status), comp_desc.vendor_err));
					return 1;
				}

				/* wr_id contain the scatter address */
				returned_value = *(uint64_t *)test_resources.atomic_data_addr.physical;

				VL_DATA_TRACE1(("Got the value from the last CMP & SWAP: "U64H_FMT,
						returned_value));

				expected_val = calc_msk_cmp_and_swap(comp_val, swap_val, mask_cs_compare, mask_cs_swap, last_val);

				/* if operation succeeded */
				if (returned_value != last_val) {
					VL_DATA_ERR(("Comp & swap FAILED:"));
					VL_DATA_ERR(("  Last value    : "U64H_FMT, last_val));
					VL_DATA_ERR(("  Compare value : "U64H_FMT, comp_val));
					VL_DATA_ERR(("  Swap value    : "U64H_FMT, swap_val));
					VL_DATA_ERR(("  Expected value: "U64H_FMT, returned_value));
					VL_DATA_ERR(("  Next expected value: "U64H_FMT, expected_val));

					return 1;
				} 

				if (expected_val != last_val)
					num_of_success ++;

				last_val = expected_val;
			}
		}
		VL_DATA_TRACE1(("Last known value in end of iteration : "U64H_FMT,
				last_val));
	}
	VL_DATA_TRACE(("Value of num_of_success is %u", num_of_success));

	return 0;
}

/***************************************
* Function: run_test
****************************************
* Description: run test 
***************************************/
static int run_test(void)
{
	int			rc = 0;
	struct VL_random_t	rand;

	/* daemon is passive */
	if (config.is_daemon) goto exit;

	config.seed = VL_srand(config.seed, &rand);

	switch (config.test_mode) {
	case MF_AND_A:
	case F_AND_A:
		rc = fetch_and_add(&rand);
		break;

	case MC_AND_S:
	case C_AND_S:
		rc = comp_and_swap(&rand);
		break;
/*
        case MF_AND_A__MC_AND_S:
            rand_masks(&rand, &mask_fa, &mask_cs_compare, &mask_cs_swap);
        case F_AND_A__C_AND_S:
            rc = fetch_and_add_AND_compare_and_swap(mask_fa, mask_cs);
            break;
*/
	default:
		VL_MISC_ERR(("%s unknown test mode %d", VL_driver_generic_name, config.test_mode));
		rc = 1;
	}

exit:

	VL_MISC_TRACE(("Run test (mode %d) - %s", config.test_mode, (rc != 0) ? "FAILED": "PASSED"));

	return rc;
}

#if 0
/***************************************
* Function: test_driver_write
***************************************/
ssize_t test_driver_write(
	IN	struct file *file,
	IN	const char *buff,
	IN	size_t count,
	IN	loff_t *offp)
{
	struct u2k_cmd_t *cmd_p = NULL;
	char *dev_name = NULL;
	size_t size;
	int rval = -EINVAL;
	int rc;

	VL_MISC_TRACE1(("%s enter write", VL_driver_generic_name));

	size = sizeof(struct u2k_cmd_t);
	cmd_p = (struct u2k_cmd_t *)kmalloc(size, GFP_KERNEL);
	if (!cmd_p) {
		VL_MEM_ERR(("%s kmalloc of %Zu bytes for command failed", VL_driver_generic_name, size));
		return -ENOMEM;
	}

	rc = copy_from_user(cmd_p, buff, size);
	if (rc) {
		VL_MISC_ERR(("%s copy_from_user of command failed", VL_driver_generic_name));
		rval = -ENOMEM;
		goto cleanup;
	}

	memcpy(&config, &cmd_p->config, sizeof(config));

	/* handle the device name */
	dev_name = kmalloc(cmd_p->dev_name_len, GFP_KERNEL);
	if (!dev_name) {
		VL_MEM_ERR(("%s kmalloc of %Zu bytes for device name failed", VL_driver_generic_name, cmd_p->dev_name_len));
		rval = -ENOMEM;
		goto cleanup;
	}

	rc = copy_from_user(dev_name, cmd_p->config.dev_name, cmd_p->dev_name_len);
	if (rc) {
		VL_MISC_ERR(("%s copy_from_user failed", VL_driver_generic_name));
		rval = -ENOMEM;
		goto cleanup;
	}

	if (cmd_p->opcode == CREATE_RESOURCES || cmd_p->opcode == FILL_QPS) {
		rc = copy_from_user(&rem_resources, cmd_p->remote_resources, sizeof(struct remote_resources_t));
		if (rc) {
			VL_MISC_ERR(("%s copy_from_user failed", VL_driver_generic_name));
			rval = -ENOMEM;
			goto cleanup;
		}
	}

	config.dev_name = dev_name;
	dev_name = NULL;
	if (cmd_p->opcode == CREATE_RESOURCES) {
		VL_MISC_TRACE(("***************************************"));
		VL_MISC_TRACE(("* HCA id                      : %s", config.dev_name));
		VL_MISC_TRACE(("* Is Daemon                   : \"%s\"", (config.is_daemon)?"YES":"NO"));
		if (!config.is_daemon)
			VL_MISC_TRACE(("* Daemon IP                   : \"%s\"", config.daemon_ip));
		VL_MISC_TRACE(("* IB port                     : %u", config.ib_port));
		VL_MISC_TRACE(("* Random seed                 : %lu", config.seed));
		VL_MISC_TRACE(("* Number of iterations        : %u", config.num_of_iter));
		VL_MISC_TRACE(("* Number of QPs               : %u", config.num_of_qps));
		VL_MISC_TRACE(("* Number of WRs               : %u", config.num_of_wrs));
		VL_MISC_TRACE(("* Test mode                   : %u", config.test_mode));
		VL_MISC_TRACE(("* Trace level                 : 0x%x", config.trace_level));
		VL_MISC_TRACE(("***************************************"));
	}

	VL_set_verbosity_level(config.trace_level);
	exit_val = -1;

	switch (cmd_p->opcode) {
	case CREATE_RESOURCES:
		init_resources();
		/* register client */
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
		/* create_resources */
		rval = create_resources();
		if (rval)
			goto cleanup;
	
		if (((config.test_mode == MF_AND_A) || (config.test_mode == MC_AND_S)) && 
			(test_resources.hca_cap.masked_atomic_cap != IB_ATOMIC_HCA)) {
			VL_MISC_ERR(("Masked atomics are not supported on this device. masked_atomic_cap: 0x%x", test_resources.hca_cap.masked_atomic_cap));
			goto cleanup;
		}

		rval = send_info2user(cmd_p);
		break;
	case FILL_QPS:
		/* fill_qps */
		rval = fill_qps();
		break;
	case RUN_TEST:
		/* Run test */
		rval = run_test();
		break;
	case DESTROY_RESOURCES:
		rval = destroy_resources();
		/* unregister client */
		if (registered)
			ib_unregister_client(&test_client);
		break;
	default:
		VL_MISC_ERR(("%s get unknown command opcode %d", VL_driver_generic_name, cmd_p->opcode));
		goto cleanup;
	}

cleanup:

	if (cmd_p)
		kfree(cmd_p);

	VL_MISC_TRACE(("%s: opcode %d - %s", VL_driver_generic_name, cmd_p->opcode, (rval) ? "FAILED": "PASSED"));

	if (rval || cmd_p->opcode == DESTROY_RESOURCES)
		VL_print_test_status(rval);

	return ((rval == 0) ? count: rval);
}
#endif
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

	dev_res.device = device;

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


/*################################################################### 
 
	KVL registeration ... 
 
 *###################################################################*/

#define USER_APP "katomic_test"
#define MODULE_NAME DRIVER_DEVICE_NAME
//extern struct config_t config;

int kvl_test_katomic(void* kvldata, void *data,void* userbuff,unsigned int len){
//	struct kvl_op* op = (struct kvl_op*)kvldata;
	struct u2k_cmd_t *cmd_p = NULL;
	char *dev_name = NULL;
	size_t size;
	int rval = -EINVAL;
	int rc;

	VL_MISC_TRACE1(("%s enter write", MODULE_NAME));

	size = sizeof(struct u2k_cmd_t);
/*	cmd_p = (struct u2k_cmd_t *)kmalloc(size, GFP_KERNEL);
	if (!cmd_p) {
		VL_MEM_ERR(("%s kmalloc of %Zu bytes for command failed", VL_driver_generic_name, size));
		return -ENOMEM;
	}
*/
	/*rc = copy_from_user(cmd_p, buff, size);
	if (rc) {
		VL_MISC_ERR(("%s copy_from_user of command failed", VL_driver_generic_name));
		rval = -ENOMEM;
		goto cleanup;
	}*/
	if (len != size) {
		VL_MEM_ERR(("len [%d] != [%ld] sizeof(struct u2k_cmd_t)\n ",len,size));
		return -EINVAL;
	}
	cmd_p = (struct u2k_cmd_t *)userbuff;

	memcpy(&config, &cmd_p->config, sizeof(config));

	/* handle the device name */
	dev_name = kmalloc(cmd_p->dev_name_len, GFP_KERNEL);
	if (!dev_name) {
		VL_MEM_ERR(("%s kmalloc of %Zu bytes for device name failed", MODULE_NAME, cmd_p->dev_name_len));
		rval = -ENOMEM;
		goto cleanup;
	}

	rc = copy_from_user(dev_name, cmd_p->config.dev_name, cmd_p->dev_name_len);
	if (rc) {
		VL_MISC_ERR(("%s copy_from_user failed", MODULE_NAME));
		rval = -ENOMEM;
		goto cleanup;
	}

	if (cmd_p->opcode == CREATE_RESOURCES || cmd_p->opcode == FILL_QPS) {
		rc = copy_from_user(&rem_resources, cmd_p->remote_resources, sizeof(struct remote_resources_t));
		if (rc) {
			VL_MISC_ERR(("%s copy_from_user failed", MODULE_NAME));
			rval = -ENOMEM;
			goto cleanup;
		}
	}

	config.dev_name = dev_name;
	dev_name = NULL;
	if (cmd_p->opcode == CREATE_RESOURCES) {
		VL_MISC_TRACE(("***************************************"));
		VL_MISC_TRACE(("* HCA id                      : %s", config.dev_name));
		VL_MISC_TRACE(("* Is Daemon                   : \"%s\"", (config.is_daemon)?"YES":"NO"));
		if (!config.is_daemon)
			VL_MISC_TRACE(("* Daemon IP                   : \"%s\"", config.daemon_ip));
		VL_MISC_TRACE(("* IB port                     : %u", config.ib_port));
		VL_MISC_TRACE(("* Random seed                 : %lu", config.seed));
		VL_MISC_TRACE(("* Number of iterations        : %u", config.num_of_iter));
		VL_MISC_TRACE(("* Number of QPs               : %u", config.num_of_qps));
		VL_MISC_TRACE(("* Number of WRs               : %u", config.num_of_wrs));
		VL_MISC_TRACE(("* Test mode                   : %u", config.test_mode));
		VL_MISC_TRACE(("* Trace level                 : 0x%x", config.trace_level));
		VL_MISC_TRACE(("***************************************"));
	}

	VL_set_verbosity_level(config.trace_level);
	exit_val = -1;

	switch (cmd_p->opcode) {
	case CREATE_RESOURCES:
		init_resources();
		/* register client */
		rc = ib_register_client(&test_client);
		if (rc) {
			VL_MISC_ERR(("%s couldn't register IB client", MODULE_NAME));
			rval = -ENODEV;
			goto cleanup;
		}
		registered = 1;
		if (exit_val) {
			VL_MISC_ERR(("%s failed because exit_val is not zero", MODULE_NAME));
			rval = -ENODEV;
			goto cleanup;
		}
		/* create_resources */
		rval = create_resources();
		if (rval)
			goto cleanup;
	
		if (((config.test_mode == MF_AND_A) || (config.test_mode == MC_AND_S)) && 
			(test_resources.hca_cap.masked_atomic_cap != IB_ATOMIC_HCA)) {
			VL_MISC_ERR(("Masked atomics are not supported on this device. masked_atomic_cap: 0x%x", test_resources.hca_cap.masked_atomic_cap));
			goto cleanup;
		}

		rval = send_info2user(cmd_p);
		break;
	case FILL_QPS:
		/* fill_qps */
		rval = fill_qps();
		break;
	case RUN_TEST:
		/* Run test */
		rval = run_test();
		break;
	case DESTROY_RESOURCES:
		rval = destroy_resources();
		/* unregister client */
		if (registered)
			ib_unregister_client(&test_client);
		break;
	default:
		VL_MISC_ERR(("%s get unknown command opcode %d", MODULE_NAME, cmd_p->opcode));
		goto cleanup;
	}

cleanup:
	
	VL_MISC_TRACE(("%s: opcode %d - %s", MODULE_NAME, cmd_p->opcode, (rval) ? "FAILED": "PASSED"));

	if (rval || cmd_p->opcode == DESTROY_RESOURCES)
		VL_print_test_status(rval);

	return rval;
}

struct kvl_op* test_katomic_op = NULL;


static int init_katomic_test(void)
{
	printk(KERN_ALERT "LOADING [%s] test ...\n",MODULE_NAME);	
	test_katomic_op = create_kvlop(MODULE_NAME,"Kernel Atomic Operation Test",MODULE_NAME,kvl_test_katomic,NULL,USER_APP); 	
	printk(KERN_ALERT "%s test was loaded\n",MODULE_NAME );
	return 0;
}

static void cleanup_katomic_test(void)
{
	printk(KERN_ALERT "UNLOADING [%s] test ...\n",MODULE_NAME);
	destroy_kvlop(test_katomic_op);
	printk(KERN_ALERT "%s test was unloaded\n",MODULE_NAME);
}


module_init(init_katomic_test);
module_exit(cleanup_katomic_test);



