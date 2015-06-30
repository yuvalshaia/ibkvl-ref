
#include <linux/kernel.h>
#include <linux/module.h>
#include <kverbs_data.h>

MODULE_LICENSE("GPL");                    
MODULE_AUTHOR("none");            
MODULE_DESCRIPTION("exercise data verbs");

/*
 * Ouer global & single thread pair. In the future we might want more
 * thread pairs to run in parallel for a given test.
 * */
static struct thread_pair tp;

/*
 * Test completion object. Each test must signal the amin thread it's
 * completion so the main thread could know when to unregister the ib
 * device and exit.
 *
 * */
DECLARE_COMPLETION(test_completion);

/*
 * thread & thread pair init / de init functions called out of the
 * thread context from the main thread. these functions init all the
 * objects that are not expected to change as we will add different
 * data loopback tets
 *
 * */
static int init_thread(struct thread_data *td, struct ib_device *ibdev, u8 port)
{
	init_completion(&td->comp);
	td->send_buf = kzalloc(BUF_LEN, GFP_KERNEL);
	if (!td->send_buf)
		goto error;
	td->recv_buf = kzalloc(BUF_LEN, GFP_KERNEL);
	if (!td->recv_buf)
		goto error;
	td->ibdev = ibdev;
	td->port = port;
	return 0;
error:
	printk(KERN_ERR "%s failed! \n", __func__);
	return 1;
}

static void deinit_thread(struct thread_data *td)
{
	/* 
 	 * do not stop our kernel thread. this thread
 	 * stop itself and if we call kthread_stop after
 	 * thread exited we get kernel crash because task
 	 * creds pointer is NULL in call to 'exit_creds'!
 	 */
	//if (td->task)
	//	kthread_stop(td->task);

	/* free resources allocated out of thread func */
	if (td->send_buf)
		kfree(td->send_buf);
	if (td->recv_buf)
		kfree(td->recv_buf);
}

int init_thread_pair(struct thread_pair *tp, struct ib_device *ibdev)
{
	init_completion(&tp->comp);

	/* init shared variables exchanged between threads in pair */
	tp->cli.remote_qpn = &tp->srv.local_qpn;
	tp->srv.remote_qpn = &tp->cli.local_qpn;
	tp->cli.remote_sq_psn = &tp->srv.local_sq_psn;
	tp->srv.remote_sq_psn = &tp->cli.local_sq_psn;
	tp->cli.remote_lid = &tp->srv.local_lid;
	tp->srv.remote_lid = &tp->cli.local_lid;

	/* for now we test data loopback on the same ib device & port */
	if (init_thread(&tp->cli, ibdev, 1))
		goto error;
	if (init_thread(&tp->srv, ibdev, 1))
		goto error;
	return 0;
error:
	printk(KERN_ERR "%s failed! \n", __func__);
	return 1;
}

void deinit_thread_pair(struct thread_pair *tp)
{
	deinit_thread(&tp->cli);
	deinit_thread(&tp->srv);
}

/*
 * verify the qp state
 * */
int verify_qp_state(struct ib_qp *qp, enum ib_qp_state required_state) 
{
	struct ib_qp_init_attr qp_init_attr;
	struct ib_qp_attr qp_attr;
	char *err = "none";
	int ret;
	int qp_attr_mask = IB_QP_STATE;

	memset(&qp_init_attr, 0, sizeof qp_init_attr);
	memset(&qp_attr, 0, sizeof qp_attr);

	if ((ret = ib_query_qp(qp, &qp_attr, qp_attr_mask, &qp_init_attr))) {
		err = "ib_query_qp reset->init";
		goto error;
	}
	if (qp_attr.qp_state != required_state)
		return 1;
	return 0;
error:
	printk(KERN_ERR "%s : %s failed! err %d \n", __func__, err, ret);
	return 1;
}

/*
 * poll cq for 1 cqe for x amount of sec
 * */
int test_poll_cq(struct ib_cq *cq, int sec)
{
	struct ib_wc wc = {0};
	int ret = 0;
	char *err = "none";
	unsigned long end_time = jiffies + HZ*sec;
	
	while (1) {
		if ((ret = ib_poll_cq(cq, 1, &wc)) < 0) {
			err = "ib_poll_cq";	
			goto error;	
		}
		if (ret == 1)
			goto found;
		if (jiffies >= end_time) {
			err = "timeout";
			goto error;
		}
		yield();
	}
found:
	if (wc.status != IB_WC_SUCCESS) {
		ret = wc.status;
		err = "wc_status";
		goto error;
	}
	return 0;
error:
	printk(KERN_ERR "%s : %s failed! err %d \n", __func__, err, ret);
	return 1;
}


/*
 * ib device init & de init functions
 * */
static void test_add_ib_device(struct ib_device *device)
{
	static struct ib_device *ibdev;
	if (ibdev) {
		printk(KERN_ERR "%s : ignore second dev! \n", __func__);
		return;
	}
	ibdev = device;
	/* run the tests on the given device */
	test_post_send(ibdev, &tp, &test_completion);
}

static void test_remove_ib_device(struct ib_device *device)
{
	/* nothing to do here. threads will fail on that ib device
	end exit */
}

struct ib_client test_client = {
	.name   = "test_data_verbs",
	.add    = test_add_ib_device,
	.remove = test_remove_ib_device
};

int kvl_test_data_verbs(void* kvldata, void *data,void* userbuff,unsigned int len){
	char *err = "none";
	if (ib_register_client(&test_client)) {
		err = "ib_register_client";
		goto error;
	}
	if (wait_for_completion_interruptible(&test_completion)) {
		err = "wait_for_completion_interruptible";
		goto error2;
	}
	ib_unregister_client(&test_client);
	printk(KERN_ERR "%s : test exited \n", __func__);
	return 0;
error2:
	ib_unregister_client(&test_client);
error:
	printk(KERN_ERR "%s : %s failed! \n", __func__, err);
	return 1;
}


/*
 * kernel module init & de init functions
 * */
struct kvl_op* test_data_verbs_op = NULL;

static int init_data_test(void)
{
	printk(KERN_ALERT "LOADING [%s] test ...\n",MODULE_NAME);	
	test_data_verbs_op = create_kvlop(MODULE_NAME, "DATA_VERBS",
		MODULE_NAME, kvl_test_data_verbs, NULL, NULL); 	
	printk(KERN_ALERT "%s test was loaded\n",MODULE_NAME );
	return 0;
}

static void cleanup_data_test(void)
{
	printk(KERN_ALERT "UNLOADING [%s] test ...\n",MODULE_NAME);
	destroy_kvlop(test_data_verbs_op);
	printk(KERN_ALERT "%s test was unloaded\n",MODULE_NAME);
}


module_init(init_data_test);
module_exit(cleanup_data_test);



