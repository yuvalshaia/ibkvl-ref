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
 * $Id: types.h 12431 2011-05-11 08:54:40Z saeedm $ 
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

#define DRIVER_DEVICE_NAME "fmr_ktest"
#define MAX_HCA_ID_LEN          256

struct config_t {
	const char*		dev_name;
	int			ib_port;
	unsigned long		seed;
	int			num_of_iter;
	int			num_of_fmr;
	int			bad_flow;
	int			num_thread_pairs;
	int			trace_level;
};


#ifdef __KERNEL__

struct resources_t {
	struct ib_device	*device;
};

struct fmr_data_t {
	struct ib_fmr_attr	fmr_props;
	struct ib_fmr		*fmr_hndl;
	u32			acl;
	u32			l_key;		/* last l_key */
	u32			r_key;		/* last r_key */
	u32			prev_l_key;	/* previous l_key */
	u32			prev_r_key;	/* previous r_key */
	int			num_of_remap;	/* number of remap since the last unmap */
};

struct thread_global_data_t {
	struct VL_thread_t		threads[2];
	volatile u32			qpns[2];
	struct semaphore		sems[2];
	volatile u32			rkeys[2];
	volatile u64			vas[2];
	volatile size_t			mr_sizes[2];
	volatile int			completed[2];
	volatile u32			acl[2];
	volatile enum ib_wr_opcode	opcode;
	volatile enum ib_wc_status	expected_status;
	struct VL_random_t		rand[2];
	int 					step[2];
};

struct validate_fmr_t {
	int			thread;
	struct ib_qp		*qp_hndl;
	struct ib_cq		*cq_hndl;
	u8			port;
	u16			dlid;
	u32			dest_qp_num;
	size_t			size;
	int			my_idx_in_pair;
};

struct modify_qp_props_t{
	int			qp_attr_mask;
	struct ib_qp_attr	qp_attr;
	struct ib_qp_cap	qp_cap;
};

struct invalidate_fmr_props_t{
	struct ib_sge		sg_entry;
	struct ib_send_wr	sr_desc;
	struct ib_wc		comp_desc;
};

struct fmr_map_t {
	u64			iova;
	u64 			*page_list;
	int			list_len;
	int			size;
};

struct thread_rsc_attribute_t {
	struct invalidate_fmr_props_t	invalidate_fmr_props;
	struct modify_qp_props_t	modify_qp_props;
	struct ib_port_attr		my_port_props;
	struct ib_port_attr		peers_port_props;
	struct ib_device_attr		hca_cap;
	struct ib_qp_init_attr		qp_init_attr;
	struct ib_wc			comp_desc;
	struct fmr_map_t		map;
	struct validate_fmr_t		validate_data;
	struct ib_sge			sg_entry;
	struct ib_send_wr		sr_desc;
	u64				pa;
	u64				va;
	size_t				effective_mr_size;
	int				num_of_pages;
	struct ib_cq			*cq_hndl;
	struct ib_qp			*qp_hndl;
	struct ib_qp			*lb_qp_hndl; /* LoopBack QP used to that the FMR is being invalidated */
	void				*buf_va;
	void				*used_va;
	struct ib_pd			*pd_hndl;
	struct list_head		fmr_hndl_list;
	int				acl;
	u32				psn;
	u32				lb_psn;
	enum ib_wc_status		last_status;
	u32				act_num_cqes;
	u32				max_fmr_pages; /* the maximum number of pages needed by any FMR */
	int				rand_num;
	struct fmr_data_t		*fmr_arr;
};



#endif /* __KERNEL__ */

#endif /* _TYPES_H_ */

