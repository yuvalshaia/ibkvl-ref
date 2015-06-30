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
 * $Id: vl_gen2k_str.h 114 2005-09-19 14:00:17Z dotanb $ 
 * 
 */

#ifndef VL_GEN2K_STR_H
#define VL_GEN2K_STR_H

#include "vl_ib_verbs.h"
#include <ib_cm.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



const char *VL_ib_device_cap_flags_str(
	INOUT		char *buf,
	IN		int bufsz,
	IN		uint32_t mask);
const char *VL_ib_atomic_cap_str(
	IN		enum ib_atomic_cap e);
const char *VL_ib_mtu_str(
	IN		enum ib_mtu e);
const char *VL_ib_port_state_str(
	IN		enum ib_port_state e);
const char *VL_ib_port_cap_flags_str(
	INOUT		char *buf,
	IN		int bufsz,
	IN		uint32_t mask);
const char *VL_ib_port_width_str(
	IN		enum ib_port_width e);
const char *VL_ib_device_modify_flags_str(
	INOUT		char *buf,
	IN		int bufsz,
	IN		uint32_t mask);
const char *VL_ib_port_modify_flags_str(
	INOUT		char *buf,
	IN		int bufsz,
	IN		uint32_t mask);
const char *VL_ib_event_type_str(
	IN		enum ib_event_type e);
const char *VL_ib_wc_status_str(
	IN		enum ib_wc_status e);
const char *VL_ib_wc_opcode_str(
	IN		enum ib_wc_opcode e);
const char *VL_ib_wc_flags_str(
	INOUT		char *buf,
	IN		int bufsz,
	IN		uint32_t mask);
const char *VL_ib_cq_notify_flags_str(
	IN		enum ib_cq_notify_flags e);
const char *VL_ib_srq_attr_mask_str(
	INOUT		char *buf,
	IN		int bufsz,
	IN		uint32_t mask);
const char *VL_ib_sig_type_str(
	IN		enum ib_sig_type e);
const char *VL_ib_qp_type_str(
	IN		enum ib_qp_type e);
const char *VL_ib_rnr_timeout_str(
	IN		enum ib_rnr_timeout e);
const char *VL_ib_qp_attr_mask_str(
	INOUT		char *buf,
	IN		int bufsz,
	IN		uint32_t mask);
const char *VL_ib_qp_state_str(
	IN		enum ib_qp_state e);
const char *VL_ib_mig_state_str(
	IN		enum ib_mig_state e);
const char *VL_ib_wr_opcode_str(
	IN		enum ib_wr_opcode e);
const char *VL_ib_send_flags_str(
	INOUT		char *buf,
	IN		int bufsz,
	IN		uint32_t mask);
const char *VL_ib_access_flags_str(
	INOUT		char *buf,
	IN		int bufsz,
	IN		uint32_t mask);
const char *VL_ib_mr_rereg_flags_str(
	INOUT		char *buf,
	IN		int bufsz,
	IN		uint32_t mask);
const char *VL_ib_process_mad_flags_str(
	INOUT		char *buf,
	IN		int bufsz,
	IN		uint32_t mask);
const char *VL_ib_mad_result_str(
	INOUT		char *buf,
	IN		int bufsz,
	IN		uint32_t mask);
void VL_ib_device_attr_print(
	IN		const struct ib_device_attr *attr);
void VL_ib_port_attr_print(
	IN		const struct ib_port_attr *attr);
void VL_ib_wc_print(
	IN		const struct ib_wc *wc);
void VL_ib_global_route_print(
	IN		const struct ib_global_route *route);
void VL_ib_qp_init_attr_print(
	IN		const struct ib_qp_init_attr *attr);
void VL_ib_qp_attr_print(
	IN		const struct ib_qp_attr *attr);
void VL_ib_srq_attr_print(
	IN		const struct ib_srq_attr *attr);
void VL_ib_send_wr_print(
	IN		const struct ib_send_wr *wr);
void VL_ib_recv_wr_print(
	IN		const struct ib_recv_wr *wr);
void VL_ib_sge_print(
	IN		const struct ib_sge *sge);
void VL_ib_qp_cap_print(
	IN		const struct ib_qp_cap *cap);
void VL_ib_ah_attr_print(
	IN		const struct ib_ah_attr *attr);
void VL_ib_gid_print(
	IN		const union ib_gid *gid);
void VL_ib_mr_attr_print(
	IN		const struct ib_mr_attr *mr_attr);


const char *VL_ib_cm_event_type(
	IN		enum ib_cm_event_type e);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* VL_GEN2K_STR_H */

