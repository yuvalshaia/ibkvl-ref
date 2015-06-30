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
 * $Id: my_types.h 10678 2010-12-13 14:55:34Z saeedm $ 
 * 
 */

#ifndef __MY_TYPES_H_
#define __MY_TYPES_H_

#include <vl.h>

#define MAX_HCA_ID_LEN          128

#define MAX_CQE (500000) //max cqe number to be crated by the test , kerenl fails to allocate device max CQEs
#define MAX_LENGTH (4*1024) //4K
#define INLINE_THRESHOLD (400)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
#define init_MUTEX(mutex) sema_init(mutex,1)
#endif

#define HCA_INFO "\
	HCA:\n\
		1. set and get client data\n\
		2. register and unregister event handler\n\
		3. dispatch event\n\
		4. query port:\n\
				legal port number\n\
		5. query GID:\n\
				legal values\n\
		6. query PKEY:\n\
				legal values\n\
		7. modify device with all of the attributes\n\
		8. modify port with all of the attributes\n\
"
#define PD_INFO "\
   PD tests:\n\
		1. alloc dealloc\n\
		2. destroy with AV\n\
		3. destroy with QP\n\
		4. destroy with MR\n\
		5. destroy with SRQ\n\
"
#define AV_INFO "\
	AV tests:\n\
			1. create destroy:\n\
					create with legal parameters\n\
			2. query AV\n\
			3. modify AV\n\
			4. create AV from WC\n\
"
#define CQ_INFO "\
	CQ tests:\n\
			1. create destroy:\n\
					create with random legal size\n\
					create with size o\n\
					cqe > HCA cap\n\
			2. resize CQ\n\
				   more than initial size\n\
					less than initial size\n\
					resize to invalid parameters\n\
			3. destroy with QP\n\
			4. modify with outstanding completions\n\
					more than number of outstanding\n\
					less than number of outstanding\n\
			5. request notification and nnotification\n\
"
#define QP_INFO "\
	QP tests:\n\
			1. create destroy:\n\
					legal random values\n\
					max WR + max SGE\n\
					RQ / SQ only\n\
			2. modify QP in all possible transitions:\n\
					a. only required parameters\n\
					b. all optional parameters\n\
					c. illegal values for required parameters\n\
					d. illegal values for optional parameters\n\
					e. not all required parameters\n\
					f. not all optional parameters\n\
					g. invalid attributes\n\
			   on good flow tests (a, b and f) also query QP\n\
			3. Query init parameters\n\
			4. Modify QP with APM parameters\n\
"	
#define MCAST_INFO "\
	MULTICAST tests:\n\
			1. attach detach:\n\
					attach detach with same legal GID & LID\n\
			2. one QP to many multicast groups\n\
			3. many QPs to one multicast group\n\
"	
#define MR_INFO "\
	MR tests:\n\
			1. reg dereg of dma MR:\n\
					reg with legal parameters\n\
			2. alloc/dealloc/map/unmap FMR:\n\
					reg with legal parameters\n\
			3. reg dereg phys MR:\n\
					reg with legal parameters\n\
"	
#define SRQ_INFO "\
	SRQ tests:\n\
			1. create destroy:\n\
					random legal size\n\
			2. modify and query SRQ to check the SRQ limit\n\
			3. try to destroy with QP\n\
			4. create the SRQ limit event\n\
"
#define POLLPOST_INFO "\
	POLL POST tests\n\
			1. ibv_post_send:\n\
					all sig types\n\
					all QP types\n\
					all legal opcodes\n\
					all legal send flags\n\
			2. ibv_post_recv:\n\
					all sig types\n\
					all QP types\n\
					RTR & RTS\n\
			3. ibv_post_srq_recv\n\
			4. ibv_poll_cq:\n\
					max CQE, and poll one at a time\n\
					max CQE, and poll random number of completions at a time\n\
"


enum test_components {
	HCA                     = 1,
	PD      	        = 1 <<  1,
	AV                      = 1 <<  2,
	CQ                  	= 1 <<  3,
	QP                      = 1 <<  4,
	MULTICAST               = 1 <<  5,
	MR                      = 1 <<  6,
	MW                      = 1 <<  7,
	SRQ                     = 1 <<  8,
	ASYNC_EVENT             = 1 <<  9,
	POLL_POST               = 1 <<  10
};

struct config_t {
	unsigned int		seed;
	char			hca_id[MAX_HCA_ID_LEN];
	int			component;
	int			test_num;
	unsigned int		leave_open : 1;
};


#ifdef __KERNEL__

#include <vl_ib_verbs.h>


struct resources {
	struct ib_device 	*device;
	struct ib_device_attr	device_attr;
	struct ib_port_attr	port_attr;
	char*			name;
	struct VL_random_t	rand_g;
	struct {
		struct ib_qp_init_attr qp_init_attr;
		struct ib_srq_init_attr srq_init_attr;
	} attributes;
	unsigned int dma_local_lkey : 1;
};

struct qp_list_node {
	struct list_head 	list;
	struct ib_qp		*qp;
};


#define GET_MAX_CQE(res) (res->device_attr.max_cqe < MAX_CQE ? res->device_attr.max_cqe : MAX_CQE)  

#define RUN_TEST(num, func) if (config->test_num == (num)) { 				\
					int rc; 						\
					rc = func(config, res, test_client);			\
					CHECK_VALUE(#func, rc, 0, return rc);			\
			     }

#define TEST_CASE(test) printk("TEST CASE:  %-30s", test);

#define PASSED  printk("        [PASSED]\n");

#define FAILED printk("        [FAILED]\n");

#define CHECK_VALUE(verb, act_val, exp_val, cmd) if ((act_val) != (exp_val)) {			\
		FAILED;										\
		VL_MISC_ERR(("Error in %s, expected value %d, actual value %d",			\
			(verb), (int)(exp_val), (int)(act_val))); 				\
		cmd; 										\
}

#define CHECK_PTR(verb, ptr, cmd) if (!(ptr)) {							\
		FAILED;										\
		VL_MISC_ERR(("Error in %s, NULL pointer returned", (verb)));			\
		cmd;										\
}

#else /* __KERNEL__ */
#define CHECK_VALUE(verb, act_val, exp_val, cmd) if ((act_val) != (exp_val)) {			\
		VL_MISC_ERR(("Error in %s, expected value %d, actual value %d",			\
			(verb), (int)(exp_val), (int)(act_val))); 				\
		cmd; 										\
}

#endif /* __KERNEL__ */

#endif /* __MY_TYPES_H_ */
