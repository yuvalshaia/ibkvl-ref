#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/fs.h>		/* for register char device */
#include <asm/uaccess.h>    /* for copy_from_user */
#include <linux/delay.h>
#include <linux/slab.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
        #include <asm/semaphore.h>      /* for semaphore */
#else
        #include <linux/semaphore.h>
#endif
#include <linux/timex.h>
#include <vl_ib_verbs.h>

#include "ksend_lat.h"
#include "ksend_lat_kernel.h"


//
// Minimal profiling support
// 
#ifdef KSL_PROF
typedef struct _prof_t
{
	cycles_t stamp;
	cycles_t sum;
	cycles_t max;
	cycles_t min;
	int num;
} prof_t;

enum {
	PROF_POST_SEND = 0,
	PROF_POST_RECV,
	PROF_COMP_SEND,
	PROF_COMP_RECV,
	PROF_LAST
};
prof_t profile_info[PROF_LAST];

#define PROF_INIT \
	memset(profile_info, 0, sizeof(profile_info))
#define PROF_START(x) \
	profile_info[x].stamp = get_cycles()
#define PROF_END(x) \
	profile_info[x].stamp = get_cycles() - profile_info[x].stamp; \
	if (profile_info[x].stamp > profile_info[x].max) \
		profile_info[x].max = profile_info[x].stamp; \
	if (profile_info[x].stamp < profile_info[x].min || !profile_info[x].min) \
		profile_info[x].min = profile_info[x].stamp; \
	profile_info[x].sum += profile_info[x].stamp; \
	profile_info[x].num++
#define PROF_END_PRINT(x) \
	PROF_END(x); \
	printk(KSL_MSG_FMT #x " latency: %lld\n", profile_info[x].stamp)
#define PROF_PRINT(x) \
	printk(KSL_MSG_FMT #x " timings - num:%d min:%lld max:%lld avg:%d\n", \
		profile_info[x].num, \
		profile_info[x].min, \
		profile_info[x].max, \
		(u_int32_t) profile_info[x].sum / profile_info[x].num )
#else
#define PROF_INIT
#define PROF_START(x)
#define PROF_END(x)
#define PROF_PRINT(x)
#define PROF_END_PRINT(x)
#endif // KSL_PROF


//
// Definitions and types
// 

#define PSN	  0
#define DEF_Q_KEY 0x11111111
#define GRH_SIZE  40


typedef struct _resources_t {
	// Test parameters
	int deamon;
	test_type_t type;
	int iter;
	int scnt;
	int rcnt;
	cycles_t *tposted;
	int test_state;

	// General device fields
	char device_name[MAX_HCA_ID_LEN];
	struct ib_device *device;
	struct ib_device_attr device_attr;
	struct ib_port_attr port_attr;
	uint8_t ib_port;
	u_int8_t is_init;
        u_int8_t is_register;

	// QP related resources
	struct ib_pd *pd;
	struct ib_ah *ah;
	struct {
		struct ib_cq *send_cq;
		struct ib_cq *recv_cq;
		int cq_size;
	} cq_s;
	struct {
		struct ib_qp *qp;
		struct ib_qp_init_attr qp_attr;
		struct ib_qp_attr qp_modify;
		int mtu;
	} qp_s;
	int tx_depth;
	int rx_depth;
	transport_t transport;

	// Memory resources
	struct {
		struct ib_mr *mr;
		char *buf;
		dma_addr_t dma;
		int mr_size;
	} mr_s;
	int buf_size; // Allocated buffer size
	int payload_size; // May change between test invocations

	// WR resources
	struct ib_sge send_list;
	struct ib_sge recv_list;
	struct ib_send_wr send_wr;
	struct ib_recv_wr recv_wr;
	int posted_recv_bufs;

	// Interrupt based wakeups
	struct semaphore sem;
} resources_t;


//
// Prototypes
// 

static void ksl_add_device(struct ib_device *device);
static void ksl_remove_device(struct ib_device *device);
static void ksl_free_resources(void);
static void ksl_comp_handler(struct ib_cq *cq, void *cq_context);
static void ksl_comp_handler_reply(struct ib_cq *cq, void *cq_context);
static void ksl_dummy_comp_handler(struct ib_cq *cq, void *cq_context);


//
// Globals
// 

static struct ib_client ksl_client = {
        .name   = "perf",
        .add    = ksl_add_device,
        .remove = ksl_remove_device
};
static resources_t res;


///////////////////////////////////////////////////////////////
//
// Initialization and teardown
// 


void ksl_init_resources(void)
{
	memset(&res, 0, sizeof(res));
}


int ksl_init(params_t *params_ptr)
{
	int rc;

	memset(&res, 0, sizeof(res));
        res.payload_size = params_ptr->payload_size;
	res.iter = params_ptr->iter;
	res.ib_port = params_ptr->ib_port;
	res.deamon = params_ptr->deamon;
	res.type = params_ptr->type;
	res.tx_depth = params_ptr->tx_depth;
	res.rx_depth = RX_DEPTH;
	res.transport = params_ptr->transport;
	strncpy(res.device_name, params_ptr->hca_id, sizeof(res.device_name) - 1);

	DBG_MSG(KSL_MSG_FMT "Accepted parameters:\n"
		  "  device name:%s\n"
		  "  payload size:%d\n"
		  "  iter:%d\n"
		  "  ib_port:%d\n"
		  "  deamon:%d\n"
		  "  tx_depth:%d\n"
		  "  rx_depth:%d\n"
		  "  transport:%d\n",
		  res.device_name,
		  res.payload_size,
		  res.iter,
		  res.ib_port,
		  res.deamon,
		  res.tx_depth,
		  res.rx_depth,
		  res.transport);

	rc = ib_register_client(&ksl_client);
	if (rc)
		return rc;

	res.is_register = 1;

	if (!res.is_init)
		return -ENODEV;

	DBG_MSG(KSL_MSG_FMT "local lid:%d and qp:%d\n", res.port_attr.lid, res.qp_s.qp->qp_num);

	// Copy back to user relevant device feilds
	params_ptr->lid = res.port_attr.lid;
	params_ptr->qpn = res.qp_s.qp->qp_num;
	params_ptr->rkey = res.mr_s.mr->rkey;
	params_ptr->vaddr = (uint64_t) (unsigned long)(void*) res.mr_s.buf;
	if (copy_to_user(&params_ptr->user_params->lid, &params_ptr->lid, sizeof(params_ptr->lid)))
		return -EFAULT;
	if (copy_to_user(&params_ptr->user_params->qpn, &params_ptr->qpn, sizeof(params_ptr->qpn)))
		return -EFAULT;
	if (copy_to_user(&params_ptr->user_params->rkey, &params_ptr->rkey, sizeof(params_ptr->rkey)))
		return -EFAULT;
	if (copy_to_user(&params_ptr->user_params->vaddr, &params_ptr->vaddr, sizeof(params_ptr->vaddr)))
		return -EFAULT;
	return 0;
}


void ksl_cleanup(void)
{
	if (res.is_register) {
		ib_unregister_client(&ksl_client);
		res.is_register = 0;
	}
}


static void ksl_add_device(struct ib_device *device)
{
	ib_comp_handler handler;

	DBG_MSG(KSL_MSG_FMT "ksl_add_device called for device:%s\n", device->name);

	if (strcmp(device->name, res.device_name))
		return;
	res.device = device;

	if (res.is_init) {
		printk(KSL_ERR_FMT "Device already initiallized!!!\n");
		return;
	}

        if (ib_query_device(device, &res.device_attr)) {
		printk(KSL_ERR_FMT "ib_query_device failed\n");
		goto cleanup;
        }

	if (ib_query_port(device, res.ib_port, &res.port_attr)) {
		printk(KSL_ERR_FMT "ib_query_port failed\n");
		goto cleanup;
	}

	if (res.port_attr.state != IB_PORT_ACTIVE) {
		printk(KSL_ERR_FMT "port not up\n");
		goto cleanup;
	}

        res.pd = ib_alloc_pd(res.device);
	if (IS_ERR(res.pd)) {
		printk(KSL_ERR_FMT "ib_alloc_pd failed\n");
		goto cleanup;
	}

	res.cq_s.cq_size = (res.tx_depth > res.rx_depth) ? res.tx_depth : res.rx_depth;
	res.cq_s.send_cq = ib_create_cq(device, NULL, NULL, NULL, res.cq_s.cq_size, 0);
	if (IS_ERR(res.cq_s.send_cq)) {
		printk(KSL_ERR_FMT "ib_create_cq failed\n");
		goto cleanup;
	}

	switch (res.type) {
	case SEND_LAT_INT_RAW:
		handler = ksl_comp_handler_reply;
		break;
	case SEND_LAT_INT_NOTIF:
		handler = ksl_comp_handler;
		break;
	default:
		handler = ksl_dummy_comp_handler;
	}
	res.cq_s.recv_cq = ib_create_cq(device, handler, NULL, NULL, res.cq_s.cq_size, 0);
	if (IS_ERR(res.cq_s.recv_cq)) {
		printk(KSL_ERR_FMT "ib_create_cq failed\n");
		goto cleanup;
	}
	res.qp_s.qp_attr.send_cq = res.cq_s.send_cq;
	res.qp_s.qp_attr.recv_cq = res.cq_s.recv_cq;
	res.qp_s.qp_attr.cap.max_send_wr = res.tx_depth;
	res.qp_s.qp_attr.cap.max_recv_wr  = res.rx_depth;
	res.qp_s.qp_attr.cap.max_send_sge = 1;
	res.qp_s.qp_attr.cap.max_recv_sge = 1;
	//res.qp_s.qp_attr.cap.max_inline_data = 0;
	res.qp_s.qp_attr.cap.max_inline_data = INLINE_THRESHOLD;

	res.qp_s.qp_attr.sq_sig_type = IB_SIGNAL_REQ_WR;
	res.qp_s.qp_attr.qp_type = (TRANSPORT_RC == res.transport) ? IB_QPT_RC : IB_QPT_UD;
	res.qp_s.qp = ib_create_qp(res.pd, &res.qp_s.qp_attr);
	if (IS_ERR(res.qp_s.qp)) {
		printk(KSL_ERR_FMT "ib_create_qp failed\n");
		goto cleanup;
	}

	printk(KSL_MSG_FMT "max inline data %d\n",res.qp_s.qp_attr.cap.max_inline_data);

	res.qp_s.qp_modify.qp_state = IB_QPS_INIT;
	res.qp_s.qp_modify.pkey_index = 0;
	res.qp_s.qp_modify.port_num = res.ib_port;
	res.qp_s.qp_modify.qp_access_flags = IB_ACCESS_REMOTE_WRITE | IB_ACCESS_REMOTE_READ;
	res.qp_s.qp_modify.qkey = DEF_Q_KEY; // only for UD
	if (TRANSPORT_RC == res.transport) {
		if (ib_modify_qp(res.qp_s.qp, &res.qp_s.qp_modify, IB_QP_STATE | IB_QP_PKEY_INDEX | IB_QP_PORT | 
				 IB_QP_ACCESS_FLAGS)) {
			printk(KSL_ERR_FMT "ib_modify_qp to state INIT failed\n");
			goto cleanup;
		}
	} else {
		if (ib_modify_qp(res.qp_s.qp, &res.qp_s.qp_modify, IB_QP_STATE | IB_QP_PKEY_INDEX | IB_QP_PORT | 
				 IB_QP_QKEY)) {
			printk(KSL_ERR_FMT "ib_modify_qp to state INIT failed\n");
			goto cleanup;
		}
	}

	res.mr_s.mr_size = res.payload_size * 2 + GRH_SIZE; // Add GRH_SIZE for UD receive buffer
	res.buf_size = res.payload_size;
	res.mr_s.buf = kmalloc(res.mr_s.mr_size, GFP_DMA); /* half for send and half for recv */
	if (!res.mr_s.buf) {
		printk(KSL_ERR_FMT "my_kmalloc failed\n");
		goto cleanup;
	}
	res.mr_s.dma = ib_dma_map_single(device, res.mr_s.buf, res.mr_s.mr_size, DMA_BIDIRECTIONAL);
	if (ib_dma_mapping_error(device, res.mr_s.dma)) {
		printk(KSL_ERR_FMT "ib_dma_map_single failed\n");
                kfree(res.mr_s.buf);
		res.mr_s.buf = NULL;
		goto cleanup;
        }
	res.mr_s.mr = ib_get_dma_mr(res.pd, IB_ACCESS_LOCAL_WRITE | IB_ACCESS_REMOTE_WRITE | IB_ACCESS_REMOTE_READ);
	if (IS_ERR(res.mr_s.mr)) {
		printk(KSL_ERR_FMT "ib_get_dma_mr failed\n");
		goto cleanup;
	}
	// Succeeed!
	res.is_init = 1;
	return;

cleanup:
	ksl_free_resources();
}


static void ksl_free_resources(void)
{
	DBG_MSG(KSL_MSG_FMT "freeing resources\n");

	if (!IS_ERR(res.ah) && res.ah) {
		if (ib_destroy_ah(res.ah))
			printk(KSL_ERR_FMT "ib_destroy_ah failed\n");
	}

	if (!IS_ERR(res.qp_s.qp) && res.qp_s.qp) {
		if (ib_destroy_qp(res.qp_s.qp))
			printk(KSL_ERR_FMT "ib_destroy_qp failed\n");
	}

	if (!IS_ERR(res.cq_s.send_cq) && res.cq_s.send_cq) {
		if (ib_destroy_cq(res.cq_s.send_cq))
			printk(KSL_ERR_FMT "ib_destroy_cq failed\n");
	}

	if (!IS_ERR(res.cq_s.recv_cq) && res.cq_s.recv_cq) {
		if (ib_destroy_cq(res.cq_s.recv_cq))
			printk(KSL_ERR_FMT "ib_destroy_cq failed\n");
	}

	if (!IS_ERR(res.mr_s.mr) && res.mr_s.mr) {
		if (ib_dereg_mr(res.mr_s.mr))
			printk(KSL_ERR_FMT "ib_dereg_mr failed\n");
	}

	if (!IS_ERR(res.pd) && res.pd) {
		if (ib_dealloc_pd(res.pd))
			printk(KSL_ERR_FMT "ib_dealloc_pd failed\n");
	}

	if (res.mr_s.buf) {
		ib_dma_unmap_single(res.device, res.mr_s.dma, res.mr_s.mr_size, DMA_BIDIRECTIONAL);
		kfree(res.mr_s.buf);
	}
}


static void ksl_remove_device(struct ib_device *device)
{
	DBG_MSG(KSL_MSG_FMT "ksl_remove_device called for device:%s\n", device->name);

	if (strcmp(device->name, res.device_name))
		return;

	if (!res.is_init) {
		printk(KSL_ERR_FMT "Device is not initiallized!!!\n");
		return;
	}
	ksl_free_resources();
	res.is_init = 0;
}


int ksl_connect_qps(params_t *params_ptr)
{
	int rc = 0;

	if (!res.is_init) {
		printk(KSL_ERR_FMT "Device is not initiallized!!!\n");
		return -ENODEV;
	}

	DBG_MSG(KSL_MSG_FMT "connecting to remote lid:%d and qp:%d\n", params_ptr->r_lid, params_ptr->r_qpn);

	memset(&res.qp_s.qp_modify, 0, sizeof(struct ib_qp_attr));
	res.qp_s.qp_modify.qp_state			= IB_QPS_RTR;
	res.qp_s.mtu = params_ptr->mtu;
	switch (params_ptr->mtu) {
	case 256 : 
		res.qp_s.qp_modify.path_mtu		= IB_MTU_256;
		break;
	case 512 :
		res.qp_s.qp_modify.path_mtu		= IB_MTU_512;
		break;
	case 1024 :
		res.qp_s.qp_modify.path_mtu		= IB_MTU_1024;
		break;
	case 2048 :
		res.qp_s.qp_modify.path_mtu		= IB_MTU_2048;
		break;
	case 0:
		// The user lets us choose:
		if (res.device_attr.vendor_part_id == 23108) { // Tavor gives better results with this MTU */
			res.qp_s.qp_modify.path_mtu        	= IB_MTU_1024;
			res.qp_s.mtu = 1024;
			printk(KSL_MSG_FMT "Defaulting to MTU:1024\n");
		} else {
			res.qp_s.qp_modify.path_mtu        	= IB_MTU_2048;
			res.qp_s.mtu = 2048;
			printk(KSL_MSG_FMT "Defaulting to MTU:2048\n");
		}
		break;
	default:
		printk(KSL_ERR_FMT "invalid MTU\n");
		return -EINVAL;
	}

	res.qp_s.qp_modify.dest_qp_num 	    		= params_ptr->r_qpn;
	res.qp_s.qp_modify.rq_psn 			= PSN;
	res.qp_s.qp_modify.max_dest_rd_atomic   	= res.device_attr.max_qp_rd_atom;
	res.qp_s.qp_modify.min_rnr_timer 		= 12;
	res.qp_s.qp_modify.ah_attr.ah_flags		= 0;
	res.qp_s.qp_modify.ah_attr.dlid         	= params_ptr->r_lid;
	res.qp_s.qp_modify.ah_attr.sl           	= 0;
	res.qp_s.qp_modify.ah_attr.src_path_bits 	= 0;
	res.qp_s.qp_modify.ah_attr.static_rate   	= 0;
	res.qp_s.qp_modify.ah_attr.port_num      	= res.ib_port;


	if (TRANSPORT_RC == res.transport) {
		rc = ib_modify_qp(res.qp_s.qp, &res.qp_s.qp_modify, IB_QP_STATE | IB_QP_AV | IB_QP_PATH_MTU | IB_QP_DEST_QPN |
				  IB_QP_RQ_PSN | IB_QP_MAX_DEST_RD_ATOMIC | IB_QP_MIN_RNR_TIMER); 
	} else {
		rc = ib_modify_qp(res.qp_s.qp, &res.qp_s.qp_modify, IB_QP_STATE); 
	}

	if (rc) {
		printk(KSL_ERR_FMT "ib_modify_qp to state RTR failed\n");
		return rc;
	}

	res.qp_s.qp_modify.qp_state 	    		= IB_QPS_RTS;
	res.qp_s.qp_modify.timeout 	    		= 12;
	res.qp_s.qp_modify.retry_cnt 			= 7;
	res.qp_s.qp_modify.rnr_retry 			= 6;
	res.qp_s.qp_modify.sq_psn 	    		= PSN;
	res.qp_s.qp_modify.max_rd_atomic     		= res.device_attr.max_qp_rd_atom;
	if (TRANSPORT_RC == res.transport) {
		rc = ib_modify_qp(res.qp_s.qp, &res.qp_s.qp_modify, IB_QP_STATE | IB_QP_TIMEOUT | IB_QP_RETRY_CNT | IB_QP_RNR_RETRY |
				  IB_QP_SQ_PSN | IB_QP_MAX_QP_RD_ATOMIC);
	} else {
		rc = ib_modify_qp(res.qp_s.qp, &res.qp_s.qp_modify, IB_QP_STATE | IB_QP_SQ_PSN);
	}
	if (rc) {
		printk(KSL_ERR_FMT "ib_modify_qp to state RTS failed\n");
		return rc;
	}

	if (TRANSPORT_UD == res.transport) {
		res.ah = ib_create_ah(res.pd, &res.qp_s.qp_modify.ah_attr);
		if (IS_ERR(res.ah) || !res.ah) {
			printk(KSL_ERR_FMT "ib_create_ah\n");
			rc = -EIO;
		}
	}
	return rc;
}


///////////////////////////////////////////////////////////////
//
// Tests
// 


static inline int is_bad_wc(struct ib_wc wc)
{
	if (wc.status != IB_WC_SUCCESS) {
		printk(KSL_ERR_FMT "send Failed status:%d\n", wc.status);
		printk(KSL_ERR_FMT "local syndrom is 0x%x\n", wc.vendor_err);
		printk(KSL_ERR_FMT "scnt=%d, rcnt=%d\n", res.scnt, res.rcnt);
		return 1;
	}
	return 0;
}


static void ksl_dummy_comp_handler(struct ib_cq *cq, void *cq_context)
{
	DBG_MSG(KSL_ERR_FMT "dummy completion handler called !?\n");
}


static void ksl_comp_handler(struct ib_cq *cq, void *cq_context)
{
	DBG_MSG(KSL_MSG_FMT "completion handler called\n");
	up(&res.sem);
}


static void ksl_comp_handler_reply(struct ib_cq *cq, void *cq_context)
{
	struct ib_send_wr *bad_wr_send;
	struct ib_recv_wr *bad_wr_recv;
	struct ib_wc wc;
	int rc = 0;
	int i;

	DBG_MSG(KSL_MSG_FMT "completion+reply handler called - scnt:%d rcnt:%d\n", res.scnt, res.rcnt);
	if (res.rcnt < res.iter) {
		rc = ib_poll_cq(res.cq_s.recv_cq, 1, &wc);
		if (rc < 0) {
			printk(KSL_ERR_FMT "ib_poll_cq failed\n");
			goto out;
		}

		if (!rc) {
			printk(KSL_ERR_FMT "unexpected empty completion queue\n");
			rc = -EIO;
			goto out;
		}
		if (is_bad_wc(wc))
		{
			rc = -EIO;
			goto out;
		}
		DBG_MSG(KSL_MSG_FMT "sucessful receive\n");
		res.rcnt++;

		// Rearm completion notification only if we are expecting another completion
		if (res.rcnt < res.iter) {
			rc = ib_req_notify_cq(res.cq_s.recv_cq, IB_CQ_NEXT_COMP);
			if (rc) {
				printk(KSL_ERR_FMT "ib_req_notify_cq failed\n");
				goto out;
			}
		}
	}
	else
	{
		printk(KSL_ERR_FMT "unexpected completion notification\n");
		rc = -EIO;
		goto out;
	}


	// If we have only 1 send descriptor, poll for a previous completion (if we sent one) before sending again
	// Note: normally, a previous send *must have* completed because this is a ping pong test. Although polling
	//       before sending adds latency, this is a must because we don't want to wait for HW completion after
	//       sending in interrupt context...
	if (res.tx_depth == 1 && res.scnt > 0) {
		rc = ib_poll_cq(res.cq_s.send_cq, 1, &wc);
		if (1 != rc) {
			printk(KSL_ERR_FMT "no pre-send completion notification\n");
			rc = -EIO;
			goto out;
		}
		if (is_bad_wc(wc))
		{
			rc = -EIO;
			goto out;
		}
		DBG_MSG(KSL_MSG_FMT "sucessful pre-send completion\n");
	}

	if (res.scnt < res.iter) {

		DBG_MSG(KSL_MSG_FMT "posting send descriptor\n");

		res.tposted[res.scnt] = get_cycles();
		rc = ib_post_send(res.qp_s.qp, &res.send_wr, &bad_wr_send);
		if (rc) {
			printk(KSL_ERR_FMT "ib_post_send failed %d\n", rc);
			goto out;
		}
		res.scnt++;
	}

	// If we have more than 1 send descriptor, we poll for a previous completion (if we sent one) after sending
	// to save latency.
	if (res.tx_depth > 1 && res.scnt > 1) {
		for (i = 1; i <= POLL_RETRIES; i++) {
			rc = ib_poll_cq(res.cq_s.send_cq, 1, &wc);
			if (rc < 0) {
				printk(KSL_ERR_FMT "ib_poll_cq failed\n");
				rc = -EIO;
				goto out;
			}
			if (1 == rc) {
				if (is_bad_wc(wc))
				{
					rc = -EIO;
					goto out;
				}
				DBG_MSG(KSL_MSG_FMT "sucessful post-send completion\n");
				break;
			}
			printk(KSL_MSG_FMT "WARNING: no post-send completion notification (try:%d)\n", i);
			udelay(1);
		}
		if (i > POLL_RETRIES) {
			printk(KSL_ERR_FMT "no post-send completion after %d usecs!\n", POLL_RETRIES);
			rc = -EIO;
			goto out;
		}
	}

	DBG_MSG(KSL_MSG_FMT "preposting receive descriptor\n");

	// Repost receive descriptor only now to save latency...
	rc = ib_post_recv(res.qp_s.qp, &res.recv_wr, &bad_wr_recv); 
	if (rc) {
		printk(KSL_ERR_FMT "ib_post_recv failed\n");
		goto out;
	}

	if (res.rcnt < res.iter || res.scnt < res.iter)
		return;

	// Test completed. Fall through...

out:
	res.test_state = rc;
	up(&res.sem);
}


int ksl_test_send_lat(params_t* params_ptr)
{
	int			rc = 0;
	struct ib_send_wr 	*bad_wr_send;
	struct ib_recv_wr 	*bad_wr_recv;
	struct ib_wc            wc;
	int			poll_bool = 0;
	int			count = 0;
	int			i;
	char* post_buf = res.mr_s.buf;

	PROF_INIT;

	init_MUTEX_LOCKED(&res.sem);
	res.tposted = NULL;
	res.scnt = 0;
	res.rcnt = 0;
	res.test_state = 0;
	if (params_ptr->payload_size > res.buf_size) {
		printk(KSL_ERR_FMT "test payload exceeds allocated memory\n");
		return -ENOMEM;
	}
	if (TRANSPORT_UD == params_ptr->transport && params_ptr->payload_size > res.qp_s.mtu) {
		printk(KSL_ERR_FMT "test payload exceeds MTU in UD transport\n");
		return -EIO;
	}
	res.payload_size = params_ptr->payload_size;

	res.send_list.addr     = (u64) res.mr_s.dma;
	res.send_list.length   = res.payload_size;
	res.send_list.lkey     = res.mr_s.mr->lkey;
	res.send_wr.wr_id      = 1;
	res.send_wr.sg_list    = &res.send_list;
	res.send_wr.num_sge    = 1;
	res.send_wr.opcode     = IB_WR_SEND;
	res.send_wr.send_flags = (params_ptr->type == SEND_LAT_INT_RAW) ? IB_SEND_SIGNALED : 0;

	if(res.payload_size > res.qp_s.qp_attr.cap.max_inline_data && params_ptr->send_inline){
	    printk(KSL_MSG_FMT "Inline send flag is ON and test payload exceeds inline max data of QP, sending without inline flag ...\n");
	}


	if(res.payload_size <= res.qp_s.qp_attr.cap.max_inline_data && params_ptr->send_inline){
	    printk(KSL_MSG_FMT "sending data inline ...\n");
	    res.send_wr.send_flags |= IB_SEND_INLINE;
	    //in case of inline buffer should not be dma mapped.
	    res.send_list.addr     = (u64) res.mr_s.buf;

	}else{
	    printk(KSL_MSG_FMT "sending data regular ...\n");
	}


	res.send_wr.next       = NULL;
	if (TRANSPORT_UD == res.transport) {
		res.send_wr.wr.ud.ah = res.ah;
		res.send_wr.wr.ud.remote_qpn = params_ptr->r_qpn;
		res.send_wr.wr.ud.remote_qkey = DEF_Q_KEY;
	}


	res.recv_list.addr     = (u64) (res.mr_s.dma + res.buf_size);
	res.recv_list.length   = res.buf_size + GRH_SIZE;
	res.recv_list.lkey     = res.mr_s.mr->lkey;
	res.recv_wr.wr_id      = 2;
	res.recv_wr.sg_list    = &res.recv_list;
	res.recv_wr.num_sge    = 1;
	res.recv_wr.next       = NULL;

	if (!res.is_init) {
		printk(KSL_ERR_FMT "Device is not initiallized!!!\n");
		return -ENODEV;
	}

	res.tposted = kmalloc(sizeof(cycles_t) * res.iter, GFP_KERNEL);
	if (!res.tposted) {
		printk(KSL_ERR_FMT "kmalloc failed\n");
		return -ENOMEM;
	}

	// We only pre-post receive buffers once
	if (!res.posted_recv_bufs) {
		for (i = 0; i < res.rx_depth - 1; i++) {
			DBG_MSG(KSL_MSG_FMT "preposting receive descriptor\n");
			rc = ib_post_recv(res.qp_s.qp, &res.recv_wr, &bad_wr_recv); 
			if (rc) {
				printk(KSL_ERR_FMT "initial ib_post_recv failed\n");
				goto exit;
			}
		}
		res.posted_recv_bufs = 1;
	}

	if (SEND_LAT_INT_RAW == params_ptr->type) {
		rc = ib_req_notify_cq(res.cq_s.recv_cq, IB_CQ_NEXT_COMP);
		if (rc)
		{
			printk(KSL_ERR_FMT "ib_req_notify_cq\n");
			goto exit;
		}

		/* If we are the client, post initial descriptor before starting the test (outside the interrupt handler) */
		if (!res.deamon) {
			DBG_MSG(KSL_MSG_FMT "posting initial send descriptor\n");
			res.tposted[res.scnt] = get_cycles();
			res.scnt++; // Set this before the send to insure scnt=1 before the completion handler
			            // is called.
			rc = ib_post_send(res.qp_s.qp, &res.send_wr, &bad_wr_send);
			if (rc) {
				printk(KSL_ERR_FMT "ib_post_send failed %d\n", rc);
				goto exit;
			}
		}

		// Let the interrupt handler do the job from here
		rc = down_interruptible(&res.sem);
		if (rc)
			goto exit;
		if (res.test_state) {
			rc = res.test_state;
			goto exit;
		}

		/* If we are the deamon, poll for the final send completion after the test (outside the interrupt handler) */
		if (res.deamon) {
			int i;
			for (i = 1; i <= POLL_RETRIES; i++) {
				rc = ib_poll_cq(res.cq_s.send_cq, 1, &wc);
				if (rc < 0) {
					printk(KSL_ERR_FMT "ib_poll_cq failed\n");
					rc = -EIO;
					goto exit;
				}
				if (1 == rc) {
					if (is_bad_wc(wc))
					{
						rc = -EIO;
						goto exit;
					}
					DBG_MSG(KSL_MSG_FMT "(final) sucessful post-send completion\n");
					break;
				}
				udelay(1);
			}
			if (i > POLL_RETRIES) {
				printk(KSL_ERR_FMT "no (final) post-send completion after %d usecs!\n", POLL_RETRIES);
				rc = -EIO;
				goto exit;
			}
			else {
				printk(KSL_MSG_FMT "received final post-send completion after %d usecs\n", i);
			}
		}
	} else {
		while (res.scnt < res.iter || res.rcnt < res.iter) {
			DBG_MSG(KSL_MSG_FMT "new iteration - scnt:%d rcnt:%d\n", res.scnt, res.rcnt);
			//
			// Apart from the client in the first iteration, we first receive
			//
			if (res.rcnt < res.iter && !(res.scnt < 1 && !res.deamon)) {
	
				// Repost buffer before polling so we don't account for the posting time:
				DBG_MSG(KSL_MSG_FMT "posting receive descriptor\n");
				PROF_START(PROF_POST_RECV);
				rc = ib_post_recv(res.qp_s.qp, &res.recv_wr, &bad_wr_recv); 
				PROF_END_PRINT(PROF_POST_RECV);
				if (rc) {
					printk(KSL_ERR_FMT "ib_post_recv failed\n");
					goto exit;
				}
	
				DBG_MSG(KSL_MSG_FMT "polling recv cq\n");
	
				rc = 1; // Initially try to recive by polling
				do {
					if (params_ptr->type == SEND_LAT_INT_NOTIF && !rc) {
						DBG_MSG(KSL_MSG_FMT "CQ empty - requesting notification\n");
						rc = ib_req_notify_cq(res.cq_s.recv_cq, IB_CQ_NEXT_COMP);
						if (rc) {
							printk(KSL_ERR_FMT "ib_req_notify_cq\n");
							goto exit;
						}
	
						// Catch any completions that were placed before requesting completion
						// notification
						rc = ib_poll_cq(res.cq_s.recv_cq, 1, &wc);
						if (rc)
							break;
	
						DBG_MSG(KSL_MSG_FMT "Going to sleep!\n");
	
						rc = down_interruptible(&res.sem);
						if (rc)
							goto exit;
					}
					PROF_START(PROF_COMP_RECV);
					rc = ib_poll_cq(res.cq_s.recv_cq, 1, &wc);
					PROF_END(PROF_COMP_RECV);
				}  while (0 == rc);
	
	
				if (rc < 0) {
					printk(KSL_ERR_FMT "ib_poll_cq failed\n");
					goto exit;
				}

				if (is_bad_wc(wc))
				{
					rc = -EIO;
					goto exit;
				}
				DBG_MSG(KSL_MSG_FMT "sucessful receive\n");
				res.rcnt++;
			}
	
			if (res.scnt < res.iter) {
				res.send_wr.send_flags = 0;
				if(res.payload_size <= res.qp_s.qp_attr.cap.max_inline_data && params_ptr->send_inline){
					DBG_MSG(KSL_MSG_FMT "sending data inline \n");
					res.send_wr.send_flags = IB_SEND_INLINE;
					//in case of inline buffer should not be dma mapped.
					res.send_list.addr     = (u64) res.mr_s.buf;
				}

				if (count++ == (res.tx_depth - 1) || res.scnt == (res.iter - 1)) {
					DBG_MSG(KSL_MSG_FMT "reached tx depth res.tx_depth = %d  - requesting completion\n",res.tx_depth );
					count = 0;
					poll_bool = 1;
					res.send_wr.send_flags |= IB_SEND_SIGNALED;
				}

				DBG_MSG(KSL_MSG_FMT "posting send descriptor\n");
				res.tposted[res.scnt] = get_cycles();
				*post_buf = (char) res.scnt;

				PROF_START(PROF_POST_SEND);
				rc = ib_post_send(res.qp_s.qp, &res.send_wr, &bad_wr_send);
				PROF_END_PRINT(PROF_POST_SEND);
				if (rc) {
					printk(KSL_ERR_FMT "ib_post_send failed %d\n", rc);
					goto exit;
				}
				res.scnt++;
			}
	
			if (poll_bool) {

				DBG_MSG(KSL_MSG_FMT "polling send cq\n");

				do {
					PROF_START(PROF_COMP_SEND);
					rc = ib_poll_cq(res.cq_s.send_cq, 1, &wc);
					PROF_END(PROF_COMP_SEND);
				} while (rc == 0);
	
				if (rc < 0) {
					printk(KSL_ERR_FMT "ib_poll_cq failed\n");
					goto exit;
				}
				if (is_bad_wc(wc))
				{
					rc = -EIO;
					goto exit;
				}
				poll_bool = 0;
				//res.send_wr.send_flags = ((res.payload_size < INLINE_THRESHOLD) ? IB_SEND_INLINE : 0);
				DBG_MSG(KSL_MSG_FMT "sucessful send completion\n");
			}
		}
	}
	rc = 0;
	DBG_MSG(KSL_MSG_FMT "test completed - returning results\n");
	if (copy_to_user((void*)(unsigned long)params_ptr->timing_ptr, res.tposted, res.iter * sizeof(cycles_t)))
		rc = -EFAULT;
	// Fall through...

exit:
	if (res.tposted)
		kfree(res.tposted);
	PROF_PRINT(PROF_POST_SEND);
	PROF_PRINT(PROF_POST_RECV);
	PROF_PRINT(PROF_COMP_SEND);
	PROF_PRINT(PROF_COMP_RECV);
	return rc;
}

