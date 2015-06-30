#ifndef __KVERBS_DATA_H__
#define __KVERBS_DATA_H__

#include <linux/init.h>
#include <linux/sched.h>
#include <linux/completion.h>
#include <linux/kthread.h>

#include <rdma/ib_verbs.h>

// mlxkvlincludes
#include <vl.h>
#include <vl_os.h>
#include <kvl.h>

#define MODULE_NAME "verbs_test"
#define BUF_LEN 20

/*
 * Data structure maintained for each thread in a thread pair.
 * Only sharing of information between threads in pair is the
 * remote QP number which allow one thread to communicate with
 * it pair thread. Server thread must be synchronized with ith
 * client thread and make sure the server thread exit only after
 * the client thread exit.
 *
 * */
struct thread_data
{
	/* private parameters */
	struct task_struct *task;
	struct completion comp;
	char *send_buf;
	u64 send_buf_dma;
	char *recv_buf;
	u64 recv_buf_dma;
	u8 port;	
	struct ib_device *ibdev;
	struct ib_port_attr port_attr;
	struct ib_pd *pd;
	struct ib_mr *mr;
	struct ib_cq *cq;
	struct ib_qp *qp;
	/* parameters exchanged between the peers */
	u32 *remote_qpn;
	u32 local_qpn;
	u32 *remote_sq_psn;
	u32 local_sq_psn;
	u16 local_lid;
	u16 *remote_lid;
};

/*
 * Data structure used to maintain pair of threads (client & server).
 * Synchronization between the thread pair and the test main thread is 
 * done from the server thread which must be synchronized with its client
 * thread.
 *
 * */
struct thread_pair
{
	struct completion comp;
	struct thread_data cli;
	struct thread_data srv;
};

/*
 * general helper functions
 * */
int verify_qp_state(struct ib_qp *qp, enum ib_qp_state required_state); 
int test_poll_cq(struct ib_cq *cq, int sec);
int init_thread_pair(struct thread_pair *tp, struct ib_device *ibdev);
void deinit_thread_pair(struct thread_pair *tp);

/*
 * specific tests entry points
 * */
int test_post_send(struct ib_device *ibdev, struct thread_pair *tp,
	struct completion *test_comp);

#endif // __KVERBS_DATA_H__
