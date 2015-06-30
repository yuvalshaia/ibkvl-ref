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
 * $Id: types.h 2689 2007-01-14 13:17:55Z dotanb $ 
 * 
 */
 
#ifndef _TYPES_H_
#define _TYPES_H_

#include <vl.h>


#ifdef __KERNEL__

#include <vl_ib_verbs.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
        #include <asm/semaphore.h>      /* for semaphore */
#else
        #include <linux/semaphore.h>
#endif
#endif /* __KERNEL__ */

#define DRIVER_DEVICE_NAME "atomic_test"
#define MAX_HCA_ID_LEN          256


enum cmd_opcode_t {
	CREATE_RESOURCES,
	FILL_QPS,
	RUN_TEST,
	DESTROY_RESOURCES
};

enum test_mode_t {
	F_AND_A,
	C_AND_S,
	F_AND_A__C_AND_S,
	MF_AND_A,
	MC_AND_S,
	MF_AND_A__MC_AND_S
};

struct config_t {
	const char			*dev_name;
	int				is_daemon;
	int				ib_port;
	unsigned long			seed;
	int				num_of_iter;
	int				num_of_qps;
	int				num_of_wrs;
	int				trace_level;
	enum test_mode_t		test_mode;
	const char			*daemon_ip;
	unsigned short			tcp_port;
};

struct remote_resources_t {
	uint64_t			remote_addr;
	uint32_t			rkey;
	uint16_t			lid;
	uint16_t			max_qp_rd_atom; /* maximum number of oust/rd as responder */
	uint32_t			*qp_num_arr;
} __attribute__ ((packed));

struct u2k_cmd_t {
	struct config_t			config;
	struct remote_resources_t	*remote_resources;
	enum cmd_opcode_t		opcode;
	size_t				dev_name_len;
};

#ifdef __KERNEL__

struct dev_resource_t {
	struct ib_device		*device;
};

struct buff_addr {
	volatile void			*physical;
	dma_addr_t			dma;
};

struct test_resources_t {
	struct ib_port_attr		my_port_props;
	struct ib_qp_init_attr		qp_init_attr;
	struct VL_random_t		rand;
	struct ib_device_attr		hca_cap;
	struct ib_pd			*pd_hndl;
	struct ib_mr			*mr_hndl;
	struct ib_qp			**qp_hndl_arr;
	uint32_t			*qp_num_arr;
	struct ib_cq			*cq_hndl;
	void				*buf;
	struct buff_addr		atomic_data_addr;
};


#endif /* __KERNEL__ */

#endif /* _TYPES_H_ */

