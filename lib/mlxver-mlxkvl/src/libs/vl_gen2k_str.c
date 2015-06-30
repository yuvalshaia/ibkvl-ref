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
 * $Id: vl_gen2k_str.c 844 2006-01-04 15:34:13Z dotanb $ 
 * 
 */


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include "vl_ib_verbs.h"
#include <ib_cm.h>
#include "vl_types.h"
#include "vl_trace.h"
#include "vl_gen2k_str.h"

#define CASE_SETSTR(e)  case e: s = #e; break;
static const char *UnKnown = "unknown";

/***************************************************
* Function: safe_append
***************************************************/
static char *safe_append(char *cbuf,
                         char *buf_end,
                         uint32_t mask, uint32_t flag, const char *flag_sym)
{
        if (mask & flag) {
                int l = (int)strlen(flag_sym);
                if (cbuf + l + 2 < buf_end) {
                        strcpy(cbuf, flag_sym);
                        cbuf += l;
                        *cbuf++ = '+';
                        *cbuf = '\0';
                } else {
                        cbuf = NULL;
                }
        }
        return cbuf;
}                               /* safe_append */

/***************************************************
* Function: end_mask_sym
***************************************************/
static void end_mask_sym(char *buf, char *cbuf, int bufsz)
{
        if (bufsz > 0) {
                if (buf == cbuf) {
                        *cbuf = '\0';   /* empty string */
                } else if (cbuf == 0) { /* was truncated */
                        int l = (int)strlen(buf);
                        buf[l - 1] = '>';
                }
        }
}                               /* end_mask_sym */

#define INIT_BUF_SKIP(skipped_pfx)     \
  int    skip = (int)strlen(skipped_pfx);   \
  char*  cbuf     = buf;               \
  char*  buf_end  = buf + bufsz;       \
  *buf = '\0';

#define SAFE_APPEND(e) \
  if (cbuf) { cbuf = safe_append(cbuf, buf_end, mask, e, #e + skip); }























/***************************************************
* Function: VL_ib_device_cap_flags_str
***************************************************/
const char *VL_ib_device_cap_flags_str(
	INOUT		char *buf,
	IN		int bufsz,
	IN		uint32_t mask)
{
	INIT_BUF_SKIP("IB_DEVICE_")
	SAFE_APPEND(IB_DEVICE_RESIZE_MAX_WR)
	SAFE_APPEND(IB_DEVICE_BAD_PKEY_CNTR)
	SAFE_APPEND(IB_DEVICE_BAD_QKEY_CNTR)
	SAFE_APPEND(IB_DEVICE_RAW_MULTI)
	SAFE_APPEND(IB_DEVICE_AUTO_PATH_MIG)
	SAFE_APPEND(IB_DEVICE_CHANGE_PHY_PORT)
	SAFE_APPEND(IB_DEVICE_UD_AV_PORT_ENFORCE)
	SAFE_APPEND(IB_DEVICE_CURR_QP_STATE_MOD)
	SAFE_APPEND(IB_DEVICE_SHUTDOWN_PORT)
	SAFE_APPEND(IB_DEVICE_INIT_TYPE)
	SAFE_APPEND(IB_DEVICE_PORT_ACTIVE_EVENT)
	SAFE_APPEND(IB_DEVICE_SYS_IMAGE_GUID)
	SAFE_APPEND(IB_DEVICE_RC_RNR_NAK_GEN)
	SAFE_APPEND(IB_DEVICE_SRQ_RESIZE)
	SAFE_APPEND(IB_DEVICE_N_NOTIFY_CQ)
	end_mask_sym(buf, cbuf, bufsz);
	return buf;
}
EXPORT_SYMBOL(VL_ib_device_cap_flags_str);

/***************************************************
* Function: VL_ib_atomic_cap_str
***************************************************/
const char *VL_ib_atomic_cap_str(
	IN		enum ib_atomic_cap e)
{
	const char *s = UnKnown;
	switch (e) {
	CASE_SETSTR(IB_ATOMIC_NONE)
	CASE_SETSTR(IB_ATOMIC_HCA)
	CASE_SETSTR(IB_ATOMIC_GLOB)

	default:;
	}
	return s;
}
EXPORT_SYMBOL(VL_ib_atomic_cap_str);

/***************************************************
* Function: VL_ib_mtu_str
***************************************************/
const char *VL_ib_mtu_str(
	IN		enum ib_mtu e)
{
	const char *s = UnKnown;
	switch (e) {
	CASE_SETSTR(IB_MTU_256)
	CASE_SETSTR(IB_MTU_512)
	CASE_SETSTR(IB_MTU_1024)
	CASE_SETSTR(IB_MTU_2048)
	CASE_SETSTR(IB_MTU_4096)

	default:;
	}
	return s;
}
EXPORT_SYMBOL(VL_ib_mtu_str);

/***************************************************
* Function: VL_ib_port_state_str
***************************************************/
const char *VL_ib_port_state_str(
	IN		enum ib_port_state e)
{
	const char *s = UnKnown;
	switch (e) {
	CASE_SETSTR(IB_PORT_NOP)
	CASE_SETSTR(IB_PORT_DOWN)
	CASE_SETSTR(IB_PORT_INIT)
	CASE_SETSTR(IB_PORT_ARMED)
	CASE_SETSTR(IB_PORT_ACTIVE)
	CASE_SETSTR(IB_PORT_ACTIVE_DEFER)
	default:;
	}
	return s;
}
EXPORT_SYMBOL(VL_ib_port_state_str);

/***************************************************
* Function: VL_ib_port_cap_flags_str
***************************************************/
const char *VL_ib_port_cap_flags_str(
	INOUT		char *buf,
	IN		int bufsz,
	IN		uint32_t mask)
{
	INIT_BUF_SKIP("IB_PORT_")
	SAFE_APPEND(IB_PORT_SM)
	SAFE_APPEND(IB_PORT_NOTICE_SUP)
	SAFE_APPEND(IB_PORT_TRAP_SUP)
	SAFE_APPEND(IB_PORT_OPT_IPD_SUP)
	SAFE_APPEND(IB_PORT_AUTO_MIGR_SUP)
	SAFE_APPEND(IB_PORT_SL_MAP_SUP)
	SAFE_APPEND(IB_PORT_MKEY_NVRAM)
	SAFE_APPEND(IB_PORT_PKEY_NVRAM)
	SAFE_APPEND(IB_PORT_LED_INFO_SUP)
	SAFE_APPEND(IB_PORT_SM_DISABLED)
	SAFE_APPEND(IB_PORT_SYS_IMAGE_GUID_SUP)
	SAFE_APPEND(IB_PORT_PKEY_SW_EXT_PORT_TRAP_SUP)
	SAFE_APPEND(IB_PORT_CM_SUP)
	SAFE_APPEND(IB_PORT_SNMP_TUNNEL_SUP)
	SAFE_APPEND(IB_PORT_REINIT_SUP)
	SAFE_APPEND(IB_PORT_DEVICE_MGMT_SUP)
	SAFE_APPEND(IB_PORT_VENDOR_CLASS_SUP)
	SAFE_APPEND(IB_PORT_DR_NOTICE_SUP)
	SAFE_APPEND(IB_PORT_CAP_MASK_NOTICE_SUP)
	SAFE_APPEND(IB_PORT_BOOT_MGMT_SUP)
	SAFE_APPEND(IB_PORT_LINK_LATENCY_SUP)
	SAFE_APPEND(IB_PORT_CLIENT_REG_SUP)

	end_mask_sym(buf, cbuf, bufsz);
	return buf;
}
EXPORT_SYMBOL(VL_ib_port_cap_flags_str);

/***************************************************
* Function: VL_ib_port_width_str
***************************************************/
const char *VL_ib_port_width_str(
	IN		enum ib_port_width e)
{
	const char *s = UnKnown;
	switch (e) {
	CASE_SETSTR(IB_WIDTH_1X)
	CASE_SETSTR(IB_WIDTH_4X)
	CASE_SETSTR(IB_WIDTH_8X)
	CASE_SETSTR(IB_WIDTH_12X)
	default:;
	}
	return s;
}
EXPORT_SYMBOL(VL_ib_port_width_str);

/***************************************************
* Function: VL_ib_device_modify_flags_str
***************************************************/
const char *VL_ib_device_modify_flags_str(
	INOUT		char *buf,
	IN		int bufsz,
	IN		uint32_t mask)
{
	INIT_BUF_SKIP("IB_DEVICE_MODIFY_")
	SAFE_APPEND(IB_DEVICE_MODIFY_SYS_IMAGE_GUID)
	SAFE_APPEND(IB_DEVICE_MODIFY_NODE_DESC)

	end_mask_sym(buf, cbuf, bufsz);
	return buf;
}
EXPORT_SYMBOL(VL_ib_device_modify_flags_str);

/***************************************************
* Function: VL_ib_port_modify_flags_str
***************************************************/
const char *VL_ib_port_modify_flags_str(
	INOUT		char *buf,
	IN		int bufsz,
	IN		uint32_t mask)
{
	INIT_BUF_SKIP("IB_PORT_")
	SAFE_APPEND(IB_PORT_SHUTDOWN)
	SAFE_APPEND(IB_PORT_INIT_TYPE)
	SAFE_APPEND(IB_PORT_RESET_QKEY_CNTR)

	end_mask_sym(buf, cbuf, bufsz);
	return buf;
}
EXPORT_SYMBOL(VL_ib_port_modify_flags_str);

/***************************************************
* Function: VL_ib_event_type_str
***************************************************/
const char *VL_ib_event_type_str(
	IN		enum ib_event_type e)
{
	const char *s = UnKnown;
	switch (e) {
	CASE_SETSTR(IB_EVENT_CQ_ERR)
	CASE_SETSTR(IB_EVENT_QP_FATAL)
	CASE_SETSTR(IB_EVENT_QP_REQ_ERR)
	CASE_SETSTR(IB_EVENT_QP_ACCESS_ERR)
	CASE_SETSTR(IB_EVENT_COMM_EST)
	CASE_SETSTR(IB_EVENT_SQ_DRAINED)
	CASE_SETSTR(IB_EVENT_PATH_MIG)
	CASE_SETSTR(IB_EVENT_PATH_MIG_ERR)
	CASE_SETSTR(IB_EVENT_DEVICE_FATAL)
	CASE_SETSTR(IB_EVENT_PORT_ACTIVE)
	CASE_SETSTR(IB_EVENT_PORT_ERR)
	CASE_SETSTR(IB_EVENT_LID_CHANGE)
	CASE_SETSTR(IB_EVENT_PKEY_CHANGE)
	CASE_SETSTR(IB_EVENT_SM_CHANGE)
	CASE_SETSTR(IB_EVENT_SRQ_ERR)
	CASE_SETSTR(IB_EVENT_SRQ_LIMIT_REACHED)
	CASE_SETSTR(IB_EVENT_QP_LAST_WQE_REACHED)
	CASE_SETSTR(IB_EVENT_CLIENT_REREGISTER)

	default:;
	}
	return s;
}
EXPORT_SYMBOL(VL_ib_event_type_str);

/***************************************************
* Function: VL_ib_wc_status_str
***************************************************/
const char *VL_ib_wc_status_str(
	IN		enum ib_wc_status e)
{
	const char *s = UnKnown;
	switch (e) {
	CASE_SETSTR(IB_WC_SUCCESS)
	CASE_SETSTR(IB_WC_LOC_LEN_ERR)
	CASE_SETSTR(IB_WC_LOC_QP_OP_ERR)
	CASE_SETSTR(IB_WC_LOC_EEC_OP_ERR)
	CASE_SETSTR(IB_WC_LOC_PROT_ERR)
	CASE_SETSTR(IB_WC_WR_FLUSH_ERR)
	CASE_SETSTR(IB_WC_MW_BIND_ERR)
	CASE_SETSTR(IB_WC_BAD_RESP_ERR)
	CASE_SETSTR(IB_WC_LOC_ACCESS_ERR)
	CASE_SETSTR(IB_WC_REM_INV_REQ_ERR)
	CASE_SETSTR(IB_WC_REM_ACCESS_ERR)
	CASE_SETSTR(IB_WC_REM_OP_ERR)
	CASE_SETSTR(IB_WC_RETRY_EXC_ERR)
	CASE_SETSTR(IB_WC_RNR_RETRY_EXC_ERR)
	CASE_SETSTR(IB_WC_LOC_RDD_VIOL_ERR)
	CASE_SETSTR(IB_WC_REM_INV_RD_REQ_ERR)
	CASE_SETSTR(IB_WC_REM_ABORT_ERR)
	CASE_SETSTR(IB_WC_INV_EECN_ERR)
	CASE_SETSTR(IB_WC_INV_EEC_STATE_ERR)
	CASE_SETSTR(IB_WC_FATAL_ERR)
	CASE_SETSTR(IB_WC_RESP_TIMEOUT_ERR)
	CASE_SETSTR(IB_WC_GENERAL_ERR)

	default:;
	}
	return s;
}
EXPORT_SYMBOL(VL_ib_wc_status_str);

/***************************************************
* Function: VL_ib_wc_opcode_str
***************************************************/
const char *VL_ib_wc_opcode_str(
	IN		enum ib_wc_opcode e)
{
	const char *s = UnKnown;
	switch (e) {
	CASE_SETSTR(IB_WC_SEND)
	CASE_SETSTR(IB_WC_RDMA_WRITE)
	CASE_SETSTR(IB_WC_RDMA_READ)
	CASE_SETSTR(IB_WC_COMP_SWAP)
	CASE_SETSTR(IB_WC_FETCH_ADD)
	CASE_SETSTR(IB_WC_BIND_MW)
	CASE_SETSTR(IB_WC_LSO)
	CASE_SETSTR(IB_WC_RECV)
	CASE_SETSTR(IB_WC_RECV_RDMA_WITH_IMM)

	default:;
	}
	return s;
}
EXPORT_SYMBOL(VL_ib_wc_opcode_str);

/***************************************************
* Function: VL_ib_wc_flags_str
***************************************************/
const char *VL_ib_wc_flags_str(
	INOUT		char *buf,
	IN		int bufsz,
	IN		uint32_t mask)
{
	INIT_BUF_SKIP("IBV_WC_")
	SAFE_APPEND(IB_WC_GRH)
	SAFE_APPEND(IB_WC_WITH_IMM)
	end_mask_sym(buf, cbuf, bufsz);
	return buf;
}
EXPORT_SYMBOL(VL_ib_wc_flags_str);

/***************************************************
* Function: VL_ib_cq_notify_flags_str
***************************************************/
const char *VL_ib_cq_notify_flags_str(
	IN		enum ib_cq_notify_flags e)
{
	const char *s = UnKnown;
	switch (e) {
	CASE_SETSTR(IB_CQ_SOLICITED)
	CASE_SETSTR(IB_CQ_NEXT_COMP)
	CASE_SETSTR(IB_CQ_SOLICITED_MASK)
	CASE_SETSTR(IB_CQ_REPORT_MISSED_EVENTS)

	default:;
	}
	return s;
}
EXPORT_SYMBOL(VL_ib_cq_notify_flags_str);

/***************************************************
* Function: VL_ib_srq_attr_mask_str
***************************************************/
const char *VL_ib_srq_attr_mask_str(
	INOUT		char *buf,
	IN		int bufsz,
	IN		uint32_t mask)
{
	INIT_BUF_SKIP("IB_SRQ_")
	SAFE_APPEND(IB_SRQ_MAX_WR)
	SAFE_APPEND(IB_SRQ_LIMIT)
	end_mask_sym(buf, cbuf, bufsz);
	return buf;
}
EXPORT_SYMBOL(VL_ib_srq_attr_mask_str);

/***************************************************
* Function: VL_ib_sig_type_str
***************************************************/
const char *VL_ib_sig_type_str(
	IN		enum ib_sig_type e)
{
	const char *s = UnKnown;
	switch (e) {
	CASE_SETSTR(IB_SIGNAL_ALL_WR)
	CASE_SETSTR(IB_SIGNAL_REQ_WR)

	default:;
	}
	return s;
}
EXPORT_SYMBOL(VL_ib_sig_type_str);

/***************************************************
* Function: VL_ib_qp_type_str
***************************************************/
const char *VL_ib_qp_type_str(
	IN		enum ib_qp_type e)
{
	const char *s = UnKnown;
	switch (e) {
	CASE_SETSTR(IB_QPT_SMI)
	CASE_SETSTR(IB_QPT_GSI)
	CASE_SETSTR(IB_QPT_RC)
	CASE_SETSTR(IB_QPT_UC)
	CASE_SETSTR(IB_QPT_UD)
#ifdef __QPT_XRC_
	CASE_SETSTR(IB_QPT_XRC)
#endif
	CASE_SETSTR(IB_QPT_RAW_IPV6)
#ifdef __QPT_ETHERTYPE_
    CASE_SETSTR(IB_QPT_RAW_ETHERTYPE)
#endif
#ifdef __QPT_RAW_ETY_
    CASE_SETSTR(IB_QPT_RAW_ETY)
#endif
	default:;
	}
	return s;
}
EXPORT_SYMBOL(VL_ib_qp_type_str);

/***************************************************
* Function: VL_ib_rnr_timeout_str
***************************************************/
const char *VL_ib_rnr_timeout_str(
	IN		enum ib_rnr_timeout e)
{
	const char *s = UnKnown;
	switch (e) {
	CASE_SETSTR(IB_RNR_TIMER_655_36)
	CASE_SETSTR(IB_RNR_TIMER_000_01)
	CASE_SETSTR(IB_RNR_TIMER_000_02)
	CASE_SETSTR(IB_RNR_TIMER_000_03)
	CASE_SETSTR(IB_RNR_TIMER_000_04)
	CASE_SETSTR(IB_RNR_TIMER_000_06)
	CASE_SETSTR(IB_RNR_TIMER_000_08)
	CASE_SETSTR(IB_RNR_TIMER_000_12)
	CASE_SETSTR(IB_RNR_TIMER_000_16)
	CASE_SETSTR(IB_RNR_TIMER_000_24)
	CASE_SETSTR(IB_RNR_TIMER_000_32)
	CASE_SETSTR(IB_RNR_TIMER_000_48)
	CASE_SETSTR(IB_RNR_TIMER_000_64)
	CASE_SETSTR(IB_RNR_TIMER_000_96)
	CASE_SETSTR(IB_RNR_TIMER_001_28)
	CASE_SETSTR(IB_RNR_TIMER_001_92)
	CASE_SETSTR(IB_RNR_TIMER_002_56)
	CASE_SETSTR(IB_RNR_TIMER_003_84)
	CASE_SETSTR(IB_RNR_TIMER_005_12)
	CASE_SETSTR(IB_RNR_TIMER_007_68)
	CASE_SETSTR(IB_RNR_TIMER_010_24)
	CASE_SETSTR(IB_RNR_TIMER_015_36)
	CASE_SETSTR(IB_RNR_TIMER_020_48)
	CASE_SETSTR(IB_RNR_TIMER_030_72)
	CASE_SETSTR(IB_RNR_TIMER_040_96)
	CASE_SETSTR(IB_RNR_TIMER_061_44)
	CASE_SETSTR(IB_RNR_TIMER_081_92)
	CASE_SETSTR(IB_RNR_TIMER_122_88)
	CASE_SETSTR(IB_RNR_TIMER_163_84)
	CASE_SETSTR(IB_RNR_TIMER_245_76)
	CASE_SETSTR(IB_RNR_TIMER_327_68)
	CASE_SETSTR(IB_RNR_TIMER_491_52)

	default:;
	}
	return s;
}
EXPORT_SYMBOL(VL_ib_rnr_timeout_str);

/***************************************************
* Function: VL_ib_qp_attr_mask_str
***************************************************/
const char *VL_ib_qp_attr_mask_str(
	INOUT		char *buf,
	IN		int bufsz,
	IN		uint32_t mask)
{
	INIT_BUF_SKIP("IB_QP_")
	SAFE_APPEND(IB_QP_STATE)
	SAFE_APPEND(IB_QP_CUR_STATE)
	SAFE_APPEND(IB_QP_EN_SQD_ASYNC_NOTIFY)
	SAFE_APPEND(IB_QP_ACCESS_FLAGS)
	SAFE_APPEND(IB_QP_PKEY_INDEX)
	SAFE_APPEND(IB_QP_PORT)
	SAFE_APPEND(IB_QP_QKEY)
	SAFE_APPEND(IB_QP_AV)
	SAFE_APPEND(IB_QP_PATH_MTU)
	SAFE_APPEND(IB_QP_TIMEOUT)
	SAFE_APPEND(IB_QP_RETRY_CNT)
	SAFE_APPEND(IB_QP_RNR_RETRY)
	SAFE_APPEND(IB_QP_RQ_PSN)
	SAFE_APPEND(IB_QP_MAX_QP_RD_ATOMIC)
	SAFE_APPEND(IB_QP_ALT_PATH)
	SAFE_APPEND(IB_QP_MIN_RNR_TIMER)
	SAFE_APPEND(IB_QP_SQ_PSN)
	SAFE_APPEND(IB_QP_MAX_DEST_RD_ATOMIC)
	SAFE_APPEND(IB_QP_PATH_MIG_STATE)
	SAFE_APPEND(IB_QP_CAP)
	SAFE_APPEND(IB_QP_DEST_QPN)
	end_mask_sym(buf, cbuf, bufsz);
	return buf;
}
EXPORT_SYMBOL(VL_ib_qp_attr_mask_str);

/***************************************************
* Function: VL_ib_qp_state_str
***************************************************/
const char *VL_ib_qp_state_str(
	IN		enum ib_qp_state e)
{
	const char *s = UnKnown;
	switch (e) {
	CASE_SETSTR(IB_QPS_RESET)
	CASE_SETSTR(IB_QPS_INIT)
	CASE_SETSTR(IB_QPS_RTR)
	CASE_SETSTR(IB_QPS_RTS)
	CASE_SETSTR(IB_QPS_SQD)
	CASE_SETSTR(IB_QPS_SQE)
	CASE_SETSTR(IB_QPS_ERR)

	default:;
	}
	return s;
}
EXPORT_SYMBOL(VL_ib_qp_state_str);

/***************************************************
* Function: VL_ib_mig_state_str
***************************************************/
const char *VL_ib_mig_state_str(
	IN		enum ib_mig_state e)
{
	const char *s = UnKnown;
	switch (e) {
	CASE_SETSTR(IB_MIG_MIGRATED)
	CASE_SETSTR(IB_MIG_REARM)
	CASE_SETSTR(IB_MIG_ARMED)

	default:;
	}
	return s;
}
EXPORT_SYMBOL(VL_ib_mig_state_str);

/***************************************************
* Function: VL_ib_wr_opcode_str
***************************************************/
const char *VL_ib_wr_opcode_str(
	IN		enum ib_wr_opcode e)
{
	const char *s = UnKnown;
	switch (e) {
	CASE_SETSTR(IB_WR_RDMA_WRITE)
	CASE_SETSTR(IB_WR_RDMA_WRITE_WITH_IMM)
	CASE_SETSTR(IB_WR_SEND)
	CASE_SETSTR(IB_WR_LSO)
	CASE_SETSTR(IB_WR_SEND_WITH_IMM)
	CASE_SETSTR(IB_WR_RDMA_READ)
	CASE_SETSTR(IB_WR_ATOMIC_CMP_AND_SWP)
	CASE_SETSTR(IB_WR_ATOMIC_FETCH_AND_ADD)

	default:;
	}
	return s;
}
EXPORT_SYMBOL(VL_ib_wr_opcode_str);

/***************************************************
* Function: VL_ib_send_flags_str
***************************************************/
const char *VL_ib_send_flags_str(
	INOUT		char *buf,
	IN		int bufsz,
	IN		uint32_t mask)
{
	INIT_BUF_SKIP("IB_SEND_")
	SAFE_APPEND(IB_SEND_FENCE)
	SAFE_APPEND(IB_SEND_SIGNALED)
	SAFE_APPEND(IB_SEND_SOLICITED)
	SAFE_APPEND(IB_SEND_INLINE)

	end_mask_sym(buf, cbuf, bufsz);
	return buf;
}
EXPORT_SYMBOL(VL_ib_send_flags_str);

/***************************************************
* Function: VL_ib_access_flags_str
***************************************************/
const char *VL_ib_access_flags_str(
	INOUT		char *buf,
	IN		int bufsz,
	IN		uint32_t mask)
{
	INIT_BUF_SKIP("IB_ACCESS_")
	SAFE_APPEND(IB_ACCESS_LOCAL_WRITE)
	SAFE_APPEND(IB_ACCESS_REMOTE_WRITE)
	SAFE_APPEND(IB_ACCESS_REMOTE_READ)
	SAFE_APPEND(IB_ACCESS_REMOTE_ATOMIC)
	SAFE_APPEND(IB_ACCESS_MW_BIND)

	end_mask_sym(buf, cbuf, bufsz);
	return buf;
}
EXPORT_SYMBOL(VL_ib_access_flags_str);

/***************************************************
* Function: VL_ib_mr_rereg_flags_str
***************************************************/
const char *VL_ib_mr_rereg_flags_str(
	INOUT		char *buf,
	IN		int bufsz,
	IN		uint32_t mask)
{
	INIT_BUF_SKIP("IB_MR_REREG_")
	SAFE_APPEND(IB_MR_REREG_TRANS)
	SAFE_APPEND(IB_MR_REREG_PD)
	SAFE_APPEND(IB_MR_REREG_ACCESS)

	end_mask_sym(buf, cbuf, bufsz);
	return buf;
}
EXPORT_SYMBOL(VL_ib_mr_rereg_flags_str);

/***************************************************
* Function: VL_ib_process_mad_flags_str
***************************************************/
const char *VL_ib_process_mad_flags_str(
	INOUT		char *buf,
	IN		int bufsz,
	IN		uint32_t mask)
{
	INIT_BUF_SKIP("IB_MAD_")
	SAFE_APPEND(IB_MAD_IGNORE_MKEY)
	SAFE_APPEND(IB_MAD_IGNORE_BKEY)

	end_mask_sym(buf, cbuf, bufsz);
	return buf;
}
EXPORT_SYMBOL(VL_ib_process_mad_flags_str);

/***************************************************
* Function: VL_ib_mad_result_str
***************************************************/
const char *VL_ib_mad_result_str(
	INOUT		char *buf,
	IN		int bufsz,
	IN		uint32_t mask)
{
	INIT_BUF_SKIP("IB_MAD_")
	if (mask == IB_MAD_RESULT_FAILURE)
                return "FAILURE";
	SAFE_APPEND(IB_MAD_RESULT_SUCCESS)
	SAFE_APPEND(IB_MAD_RESULT_REPLY)
	SAFE_APPEND(IB_MAD_RESULT_CONSUMED)

	end_mask_sym(buf, cbuf, bufsz);
	return buf;
}
EXPORT_SYMBOL(VL_ib_mad_result_str);

/***************************************************
* Function: VL_ib_device_attr_print
***************************************************/
void VL_ib_device_attr_print(
	IN		const struct ib_device_attr *attr)
{
	VL_MISC_TRACE(("fw_ver                       "U64H_FMT, attr->fw_ver));
	VL_MISC_TRACE(("sys_image_guid               "U64H_FMT, attr->sys_image_guid));
	VL_MISC_TRACE(("max_mr_size                  "U64H_FMT, attr->max_mr_size));
	VL_MISC_TRACE(("page_size_cap                "U64H_FMT, attr->page_size_cap));
	VL_MISC_TRACE(("vendor_id                    0x%x", attr->vendor_id));
	VL_MISC_TRACE(("vendor_part_id               0x%x", attr->vendor_part_id));
	VL_MISC_TRACE(("hw_ver                       0x%x", attr->hw_ver));
	VL_MISC_TRACE(("max_qp                       0x%x", attr->max_qp));
	VL_MISC_TRACE(("max_qp_wr                    0x%x", attr->max_qp_wr));
	VL_MISC_TRACE(("device_cap_flags             0x%x", attr->device_cap_flags));
	VL_MISC_TRACE(("max_sge                      0x%x", attr->max_sge));
	VL_MISC_TRACE(("max_sge_rd                   0x%x", attr->max_sge_rd));
	VL_MISC_TRACE(("max_cq                       0x%x", attr->max_cq));
	VL_MISC_TRACE(("max_cqe                      0x%x", attr->max_cqe));
	VL_MISC_TRACE(("max_mr                       0x%x", attr->max_mr));
	VL_MISC_TRACE(("max_pd                       0x%x", attr->max_pd));
	VL_MISC_TRACE(("max_qp_rd_atom               0x%x", attr->max_qp_rd_atom));
	VL_MISC_TRACE(("max_ee_rd_atom               0x%x", attr->max_ee_rd_atom));
	VL_MISC_TRACE(("max_res_rd_atom              0x%x", attr->max_res_rd_atom));
	VL_MISC_TRACE(("max_qp_init_rd_atom          0x%x", attr->max_qp_init_rd_atom));
	VL_MISC_TRACE(("max_ee_init_rd_atom          0x%x", attr->max_ee_init_rd_atom));
	VL_MISC_TRACE(("atomic_cap                   %s", VL_ib_atomic_cap_str(attr->atomic_cap)));
	VL_MISC_TRACE(("max_ee                       0x%x", attr->max_ee));
	VL_MISC_TRACE(("max_rdd                      0x%x", attr->max_rdd));
	VL_MISC_TRACE(("max_mw                       0x%x", attr->max_mw));
	VL_MISC_TRACE(("max_raw_ipv6_qp              0x%x", attr->max_raw_ipv6_qp));
	VL_MISC_TRACE(("max_raw_ethy_qp              0x%x", attr->max_raw_ethy_qp));
	VL_MISC_TRACE(("max_mcast_grp                0x%x", attr->max_mcast_grp));
	VL_MISC_TRACE(("max_mcast_qp_attach          0x%x", attr->max_mcast_qp_attach));
	VL_MISC_TRACE(("max_total_mcast_qp_attach    0x%x", attr->max_total_mcast_qp_attach));
	VL_MISC_TRACE(("max_ah                       0x%x", attr->max_ah));
	VL_MISC_TRACE(("max_fmr                      0x%x", attr->max_fmr));
	VL_MISC_TRACE(("max_map_per_fmr              0x%x", attr->max_map_per_fmr));
	VL_MISC_TRACE(("max_srq                      0x%x", attr->max_srq));
	VL_MISC_TRACE(("max_srq_wr                   0x%x", attr->max_srq_wr));
	VL_MISC_TRACE(("max_srq_sge                  0x%x", attr->max_srq_sge));
	VL_MISC_TRACE(("max_pkeys                    0x%x", attr->max_pkeys));
	VL_MISC_TRACE(("local_ca_ack_delay           0x%x", attr->local_ca_ack_delay));
}
EXPORT_SYMBOL(VL_ib_device_attr_print);

/***************************************************
* Function: VL_ib_port_attr_print
***************************************************/
void VL_ib_port_attr_print(
	IN		const struct ib_port_attr *attr)
{
	VL_MISC_TRACE(("state                        %s", VL_ib_port_state_str(attr->state)));
	VL_MISC_TRACE(("max_mtu                      %s", VL_ib_mtu_str(attr->max_mtu)));
	VL_MISC_TRACE(("active_mtu                   %s", VL_ib_mtu_str(attr->max_mtu)));
	VL_MISC_TRACE(("gid_tbl_len                  0x%x", attr->gid_tbl_len));
	VL_MISC_TRACE(("port_cap_flags               0x%x", attr->port_cap_flags));
	VL_MISC_TRACE(("max_msg_sz                   0x%x", attr->max_msg_sz));
	VL_MISC_TRACE(("bad_pkey_cntr                0x%x", attr->bad_pkey_cntr));
	VL_MISC_TRACE(("qkey_viol_cntr               0x%x", attr->qkey_viol_cntr));
	VL_MISC_TRACE(("pkey_tbl_len                 0x%x", attr->pkey_tbl_len));
	VL_MISC_TRACE(("lid                          0x%x", attr->lid));
	VL_MISC_TRACE(("sm_lid                       0x%x", attr->sm_lid));
	VL_MISC_TRACE(("lmc                          0x%x", attr->lmc));
	VL_MISC_TRACE(("max_vl_num                   0x%x", attr->max_vl_num));
	VL_MISC_TRACE(("sm_sl                        0x%x", attr->sm_sl));
	VL_MISC_TRACE(("subnet_timeout               0x%x", attr->subnet_timeout));
	VL_MISC_TRACE(("init_type_reply              0x%x", attr->init_type_reply));
	VL_MISC_TRACE(("active_width                 0x%x", attr->active_width));
	VL_MISC_TRACE(("active_speed                 0x%x", attr->active_speed));
	VL_MISC_TRACE(("phys_state                   0x%x", attr->phys_state));
}
EXPORT_SYMBOL(VL_ib_port_attr_print);

/***************************************************
* Function: VL_ib_wc_print
***************************************************/
void VL_ib_wc_print(
	IN		const struct ib_wc *wc)
{
	VL_MISC_TRACE(("wr_id                        "U64H_FMT, wc->wr_id));
	VL_MISC_TRACE(("status                       %s", VL_ib_wc_status_str(wc->status)));
	VL_MISC_TRACE(("opcode                       %s", VL_ib_wc_opcode_str(wc->opcode)));
	VL_MISC_TRACE(("vendor_err                   0x%x", wc->vendor_err));
	VL_MISC_TRACE(("byte_len                     0x%x", wc->byte_len));
	VL_MISC_TRACE(("imm_data                     0x%x", wc->ex.imm_data));
	//VL_MISC_TRACE(("qp_num                       0x%x", wc->qp_num)); -> todo 
	VL_MISC_TRACE(("src_qp                       0x%x", wc->src_qp));
	VL_MISC_TRACE(("wc_flags                     0x%x", wc->wc_flags)); /* enum ib_wc_flags */
	VL_MISC_TRACE(("pkey_index                   0x%x", wc->pkey_index));
	VL_MISC_TRACE(("slid                         0x%x", wc->slid));
	VL_MISC_TRACE(("sl                           0x%x", wc->sl));
	VL_MISC_TRACE(("dlid_path_bits               0x%x", wc->dlid_path_bits));
	VL_MISC_TRACE(("port_num                     0x%x", wc->port_num));
}
EXPORT_SYMBOL(VL_ib_wc_print);

/***************************************************
* Function: VL_ib_global_route_print
***************************************************/
void VL_ib_global_route_print(
	IN		const struct ib_global_route *route)
{
	VL_ib_gid_print(&route->dgid);
	VL_MISC_TRACE(("flow_label                   0x%x", route->flow_label));
	VL_MISC_TRACE(("sgid_index                   0x%x", route->sgid_index));
	VL_MISC_TRACE(("hop_limit                    0x%x", route->hop_limit));
	VL_MISC_TRACE(("traffic_class                0x%x", route->traffic_class));
}
EXPORT_SYMBOL(VL_ib_global_route_print);

/***************************************************
* Function: VL_ib_qp_init_attr_print
***************************************************/
void VL_ib_qp_init_attr_print(
	IN		const struct ib_qp_init_attr *attr)
{
	VL_ib_qp_cap_print(&attr->cap);
	VL_MISC_TRACE(("sq_sig_type                  0x%x\n", attr->sq_sig_type));
	VL_MISC_TRACE(("qp_type                      %s\n", VL_ib_qp_type_str(attr->qp_type)));
	VL_MISC_TRACE(("port_num                     0x%x\n", attr->port_num));
}
EXPORT_SYMBOL(VL_ib_qp_init_attr_print);

/***************************************************
* Function: VL_ib_qp_attr_print
***************************************************/
void VL_ib_qp_attr_print(
	IN		const struct ib_qp_attr *attr)
{
	VL_MISC_TRACE(("qp_state                     %s", VL_ib_qp_state_str(attr->qp_state)));
	VL_MISC_TRACE(("cur_qp_state                 %s", VL_ib_qp_state_str(attr->qp_state)));
	VL_MISC_TRACE(("path_mtu                     %s", VL_ib_mtu_str(attr->path_mtu)));
	VL_MISC_TRACE(("path_mig_state               %s", VL_ib_mig_state_str(attr->path_mig_state)));
	VL_MISC_TRACE(("qkey                         0x%x", attr->qkey));
	VL_MISC_TRACE(("rq_psn                       0x%x", attr->rq_psn));
	VL_MISC_TRACE(("sq_psn                       0x%x", attr->sq_psn));
	VL_MISC_TRACE(("dest_qp_num                  0x%x", attr->dest_qp_num));
	VL_MISC_TRACE(("qp_access_flags              0x%x", attr->qp_access_flags));
	VL_ib_qp_cap_print(&attr->cap);
	VL_ib_ah_attr_print(&attr->ah_attr);
	VL_ib_ah_attr_print(&attr->alt_ah_attr);
	VL_MISC_TRACE(("pkey_index                   0x%x", attr->pkey_index));
	VL_MISC_TRACE(("alt_pkey_index               0x%x", attr->alt_pkey_index));
	VL_MISC_TRACE(("en_sqd_async_notify          0x%x", attr->en_sqd_async_notify));
	VL_MISC_TRACE(("sq_draining                  0x%x", attr->sq_draining));
	VL_MISC_TRACE(("max_rd_atomic                0x%x", attr->max_rd_atomic));
	VL_MISC_TRACE(("max_dest_rd_atomic           0x%x", attr->max_dest_rd_atomic));
	VL_MISC_TRACE(("min_rnr_timer                0x%x", attr->min_rnr_timer));
	VL_MISC_TRACE(("port_num                     0x%x", attr->port_num));
	VL_MISC_TRACE(("timeout                      0x%x", attr->timeout));
	VL_MISC_TRACE(("retry_cnt                    0x%x", attr->retry_cnt));
	VL_MISC_TRACE(("rnr_retry                    0x%x", attr->rnr_retry));
	VL_MISC_TRACE(("alt_port_num                 0x%x", attr->alt_port_num));
	VL_MISC_TRACE(("alt_timeout                  0x%x", attr->alt_timeout));
}
EXPORT_SYMBOL(VL_ib_qp_attr_print);

/***************************************************
* Function: VL_ib_srq_attr_print
***************************************************/
void VL_ib_srq_attr_print(
	IN		const struct ib_srq_attr *attr)
{
	VL_MISC_TRACE(("max_wr                       0x%x", attr->max_wr));
	VL_MISC_TRACE(("max_sge                      0x%x", attr->max_sge));
	VL_MISC_TRACE(("srq_limit                    0x%x", attr->srq_limit));
}
EXPORT_SYMBOL(VL_ib_srq_attr_print);

/***************************************************
* Function: VL_ib_send_wr_print
***************************************************/
void VL_ib_send_wr_print(
	IN		const struct ib_send_wr *wr)
{
	int i;


	VL_MISC_TRACE(("next                         0x%p", wr->next));
	VL_MISC_TRACE(("wr_id                        "U64H_FMT, wr->wr_id));
	for (i = 0; i < wr->num_sge; i ++) {
		VL_MISC_TRACE(("s/g[%d]", i));
		VL_ib_sge_print(&wr->sg_list[i]);
	}
	VL_MISC_TRACE(("num_sge                      0x%x", wr->num_sge));
	VL_MISC_TRACE(("opcode                       %s", VL_ib_wr_opcode_str(wr->opcode))); /* enum ibv_wr_opcode */
	VL_MISC_TRACE(("send_flags                   0x%x", wr->send_flags)); /* enum ibv_send_flags	*/
	if ((wr->opcode == IB_WR_RDMA_WRITE_WITH_IMM) || 
		(wr->opcode == IB_WR_SEND_WITH_IMM)) {
		VL_MISC_TRACE(("imm_data                     0x%x", wr->ex.imm_data));
	}
	
	if ((wr->opcode == IB_WR_RDMA_WRITE) ||
		(wr->opcode == IB_WR_RDMA_WRITE_WITH_IMM) ||
		(wr->opcode == IB_WR_RDMA_READ) ||
		(wr->opcode == IB_WR_ATOMIC_CMP_AND_SWP) ||
		(wr->opcode == IB_WR_ATOMIC_FETCH_AND_ADD)) {
		VL_MISC_TRACE(("remote_addr                  "U64H_FMT, wr->wr.rdma.remote_addr));
		VL_MISC_TRACE(("rkey;                        0x%x", wr->wr.rdma .rkey));
	}

	if ((wr->opcode == IB_WR_ATOMIC_CMP_AND_SWP) ||
		(wr->opcode == IB_WR_ATOMIC_FETCH_AND_ADD)) {
		VL_MISC_TRACE(("compare_add                  "U64H_FMT, wr->wr.atomic.compare_add));
	}
		
	if (wr->opcode == IB_WR_ATOMIC_CMP_AND_SWP)
		VL_MISC_TRACE(("swap                         "U64H_FMT, wr->wr.atomic.swap));

	if (((wr->opcode == IB_WR_SEND) ||
		(wr->opcode == IB_WR_SEND_WITH_IMM)) && (wr->wr.ud.ah != NULL)) {
/*
		VL_ibv_ah_print(wr->wr.ud.ah);
*/

		/* struct ib_mad_hdr *mad_hdr; */

		VL_MISC_TRACE(("remote_qpn                   0x%x", wr->wr.ud.remote_qpn));
		VL_MISC_TRACE(("remote_qkey                  0x%x", wr->wr.ud.remote_qkey));

		VL_MISC_TRACE(("pkey_index(GSI)              0x%x", wr->wr.ud.pkey_index));
		VL_MISC_TRACE(("port_num(DR SMPs on swith)   0x%x", wr->wr.ud.port_num));
	}
}
EXPORT_SYMBOL(VL_ib_send_wr_print);

/***************************************************
* Function: VL_ib_recv_wr_print
***************************************************/
void VL_ib_recv_wr_print(
	IN		const struct ib_recv_wr *wr)
{
	int i;


	VL_MISC_TRACE(("next                         0x%p", wr->next));
	VL_MISC_TRACE(("wr_id                        "U64H_FMT, wr->wr_id));
	for (i = 0; i < wr->num_sge; i ++) {
		VL_MISC_TRACE(("s/g[%d]", i));
		VL_ib_sge_print(&wr->sg_list[i]);
	}
	VL_MISC_TRACE(("num_sge                      0x%x", wr->num_sge));
}
EXPORT_SYMBOL(VL_ib_recv_wr_print);

/***************************************************
* Function: VL_ib_sge_print
***************************************************/
void VL_ib_sge_print(
	IN		const struct ib_sge *sge)
{
	VL_MISC_TRACE(("addr                         "U64H_FMT, sge->addr));
	VL_MISC_TRACE(("length                       0x%x", sge->length));
	VL_MISC_TRACE(("lkey                         0x%x", sge->lkey));
}
EXPORT_SYMBOL(VL_ib_sge_print);

/***************************************************
* Function: VL_ib_qp_cap_print
***************************************************/
void VL_ib_qp_cap_print(
	IN		const struct ib_qp_cap *cap)
{
	VL_MISC_TRACE(("max_send_wr                  0x%x", cap->max_send_wr));
	VL_MISC_TRACE(("max_recv_wr                  0x%x", cap->max_recv_wr));
	VL_MISC_TRACE(("max_send_sge                 0x%x", cap->max_send_sge));
	VL_MISC_TRACE(("max_recv_sge                 0x%x", cap->max_recv_sge));
	VL_MISC_TRACE(("max_inline_data              0x%x", cap->max_inline_data));
}
EXPORT_SYMBOL(VL_ib_qp_cap_print);

/***************************************************
* Function: VL_ib_ah_attr_print
***************************************************/
void VL_ib_ah_attr_print(
	IN		const struct ib_ah_attr *attr)
{
	VL_ib_global_route_print(&attr->grh);
	VL_MISC_TRACE(("dlid                         0x%x", attr->dlid));
	VL_MISC_TRACE(("sl                           0x%x", attr->sl));
	VL_MISC_TRACE(("src_path_bits                0x%x", attr->src_path_bits));
	VL_MISC_TRACE(("static_rate                  0x%x", attr->static_rate));
	VL_MISC_TRACE(("ah_flags                     0x%x", attr->ah_flags));
	VL_MISC_TRACE(("port_num                     0x%x", attr->port_num));
}
EXPORT_SYMBOL(VL_ib_ah_attr_print);

/***************************************************
* Function: VL_ib_gid_print
***************************************************/
void VL_ib_gid_print(
	IN		const union ib_gid *gid)
{
    VL_MISC_TRACE(("%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u",
                   gid->raw[0], gid->raw[1], gid->raw[2], gid->raw[3],
                   gid->raw[4], gid->raw[5], gid->raw[6], gid->raw[7],
                   gid->raw[8], gid->raw[9], gid->raw[10], gid->raw[11],
                   gid->raw[12], gid->raw[13], gid->raw[14], gid->raw[15]));
}
EXPORT_SYMBOL(VL_ib_gid_print);

/***************************************************
* Function: VL_ib_mr_attr_print
***************************************************/
void VL_ib_mr_attr_print(
	IN		const struct ib_mr_attr *mr_attr)
{
	/* struct ibv_context context */
	/*VL_ib_pd_print(mr_attr->pd);*/
	VL_MISC_TRACE(("device_virt_addr             "U64H_FMT, mr_attr->device_virt_addr));
	VL_MISC_TRACE(("size                         "U64H_FMT, mr_attr->size));
	VL_MISC_TRACE(("mr_access_flags              0x%x", mr_attr->mr_access_flags));
	VL_MISC_TRACE(("lrkey                        0x%x", mr_attr->lkey));
	VL_MISC_TRACE(("rkey                         0x%x", mr_attr->rkey));
}
EXPORT_SYMBOL(VL_ib_mr_attr_print);


/***************************************************
* Function: VL_ib_cm_event_type
***************************************************/
const char *VL_ib_cm_event_type(
	IN		enum ib_cm_event_type e)
{
	const char *s = UnKnown;
	switch (e) {
	CASE_SETSTR(IB_CM_REQ_ERROR)
	CASE_SETSTR(IB_CM_REQ_RECEIVED)
	CASE_SETSTR(IB_CM_REP_ERROR)
	CASE_SETSTR(IB_CM_REP_RECEIVED)
	CASE_SETSTR(IB_CM_RTU_RECEIVED)
	CASE_SETSTR(IB_CM_USER_ESTABLISHED)
	CASE_SETSTR(IB_CM_DREQ_ERROR)
	CASE_SETSTR(IB_CM_DREQ_RECEIVED)
	CASE_SETSTR(IB_CM_DREP_RECEIVED)
	CASE_SETSTR(IB_CM_TIMEWAIT_EXIT)
	CASE_SETSTR(IB_CM_MRA_RECEIVED)
	CASE_SETSTR(IB_CM_REJ_RECEIVED)
	CASE_SETSTR(IB_CM_LAP_ERROR)
	CASE_SETSTR(IB_CM_LAP_RECEIVED)
	CASE_SETSTR(IB_CM_APR_RECEIVED)
	CASE_SETSTR(IB_CM_SIDR_REQ_ERROR)
	CASE_SETSTR(IB_CM_SIDR_REQ_RECEIVED)
	CASE_SETSTR(IB_CM_SIDR_REP_RECEIVED)

	default:;
	}
	return s;
}
EXPORT_SYMBOL(VL_ib_cm_event_type);

