
#include <kverbs_data.h>

/*
 * init / de init of test specific thread objects
 *
 * */
static int test_post_send_thread_init(struct thread_data *td)
{
	char *err = "none";
	struct ib_qp_init_attr qp_init_attr;
	struct ib_qp_attr qp_attr;
	int qp_attr_mask;
	struct ib_recv_wr wr;
	struct ib_recv_wr *bad_wr;
	struct ib_sge sge;
	int ret = 0;

	if ((ret = ib_query_port(td->ibdev, td->port, &td->port_attr))) {
		err = "ib_query_port";
		goto error;
	}

	td->local_lid = td->port_attr.lid;

	td->pd = ib_alloc_pd(td->ibdev);
	if (IS_ERR(td->pd)) {
		td->pd = NULL;
		err = "ib_alloc_pd";
		goto error;
	}
	
	td->mr = ib_get_dma_mr(td->pd, IB_ACCESS_LOCAL_WRITE);
	if (IS_ERR(td->mr)) {
		td->mr = NULL;
		err = "ib_get_dma_mr";
		goto error;
	}
	td->cq = ib_create_cq(td->ibdev, NULL, NULL, NULL, 5, 0);
	if (IS_ERR(td->cq)) {
		td->cq = NULL;
		err = "ib_create_cq";
		goto error;
	}

	memset(&qp_init_attr, 0, sizeof qp_init_attr);

	qp_init_attr.qp_context = NULL;
	qp_init_attr.send_cq = td->cq;
	qp_init_attr.recv_cq = td->cq;
	qp_init_attr.qp_type = IB_QPT_RC;
	qp_init_attr.sq_sig_type = 0;
	qp_init_attr.xrcd = NULL;
	qp_init_attr.cap.max_send_wr = 1;
	qp_init_attr.cap.max_recv_wr = 1;
	qp_init_attr.cap.max_send_sge = 1;
	qp_init_attr.cap.max_recv_sge = 1;
	qp_init_attr.cap.max_inline_data = 50;

	td->qp = ib_create_qp(td->pd, &qp_init_attr);
	if (IS_ERR(td->qp)) {
		td->qp = NULL;
		err = "ib_create_qp";
		goto error;
	}
	td->local_qpn = td->qp->qp_num;

	td->send_buf_dma = ib_dma_map_single(td->ibdev, td->send_buf,
		BUF_LEN, DMA_TO_DEVICE);
	if ((ret = ib_dma_mapping_error(td->ibdev, td->send_buf_dma))) {
		err = "send buf dma";
		goto error;
	} 	
	td->recv_buf_dma = ib_dma_map_single(td->ibdev, td->recv_buf,
		BUF_LEN, DMA_FROM_DEVICE);
	if ((ret = ib_dma_mapping_error(td->ibdev, td->recv_buf_dma))) {
		err = "recv buf dma";
		goto error;
	}
 
	memset(&qp_attr, 0, sizeof qp_attr);

	qp_attr.qp_state = IB_QPS_INIT;
	qp_attr.pkey_index = 0;
	qp_attr.port_num = 1;
	qp_attr.qp_access_flags = IB_ACCESS_LOCAL_WRITE |
		IB_ACCESS_REMOTE_WRITE | IB_ACCESS_REMOTE_READ;

	qp_attr_mask = IB_QP_STATE | IB_QP_PKEY_INDEX |
		IB_QP_PORT | IB_QP_ACCESS_FLAGS;
	
	if ((ret = ib_modify_qp(td->qp, &qp_attr, qp_attr_mask))) {
		err = "ib_modify_qp reset->init";
		goto error;
	}

	if (verify_qp_state(td->qp, IB_QPS_INIT)) {
		err = "qp not in IB_QPS_INIT";
		goto error;
	}

	memset(&sge, 0, sizeof sge);
	
	sge.addr = td->recv_buf_dma;
	sge.length = BUF_LEN;
	sge.lkey = td->mr->lkey;
	
	memset(&wr, 0, sizeof wr);

	wr.wr_id = 0;	
	wr.sg_list = &sge;	
	wr.num_sge = 1;	
	
	if ((ret = ib_post_recv(td->qp, &wr, &bad_wr))) {
		err = "ib_post_recv";
		goto error;
	}

	return 0;
error:
	printk(KERN_ERR "%s : %s failed! err %d \n", __func__, err, ret);
	return 1;
}

/*
 * init stage 2 of test specific thread objects
 * 
 **/
static int test_post_send_thread_init_2(struct thread_data *td)
{
        struct ib_qp_attr qp_attr;
	int qp_attr_mask;
	int ret;
	char *err = "none";
        
	memset(&qp_attr, 0, sizeof qp_attr);

        qp_attr.qp_state = IB_QPS_RTR;
        qp_attr.path_mtu = IB_MTU_512;
	qp_attr.dest_qp_num = *(td->remote_qpn);
	qp_attr.rq_psn = *(td->remote_sq_psn);
	qp_attr.max_dest_rd_atomic = 1;
	qp_attr.min_rnr_timer = 0;
	/* address handle attr */
	qp_attr.ah_attr.sl = 0;
	qp_attr.ah_attr.ah_flags = 0;
	qp_attr.ah_attr.dlid = *(td->remote_lid);
	qp_attr.ah_attr.static_rate = 2; /* 1x */
	qp_attr.ah_attr.src_path_bits = 0;

        qp_attr_mask = IB_QP_STATE | IB_QP_PATH_MTU | IB_QP_AV |
		IB_QP_DEST_QPN | IB_QP_RQ_PSN | IB_QP_MAX_DEST_RD_ATOMIC |
		IB_QP_MIN_RNR_TIMER; 
        
        if ((ret = ib_modify_qp(td->qp, &qp_attr, qp_attr_mask))) {
                err = "ib_modify_qp init->rtr";
                goto error;
        }

        if (verify_qp_state(td->qp, IB_QPS_RTR)) {
                err = "qp not in IB_QPS_RTR";
                goto error;
        }

	memset(&qp_attr, 0, sizeof qp_attr);

	qp_attr.qp_state = IB_QPS_RTS;
	qp_attr.timeout = 14;
	qp_attr.retry_cnt = 7;
	qp_attr.rnr_retry = 7;
	qp_attr.sq_psn = td->local_sq_psn;
	qp_attr.max_rd_atomic = 1;

        qp_attr_mask = IB_QP_STATE | IB_QP_TIMEOUT | IB_QP_RETRY_CNT |
		IB_QP_RNR_RETRY | IB_QP_SQ_PSN | IB_QP_MAX_QP_RD_ATOMIC;
	
        if ((ret = ib_modify_qp(td->qp, &qp_attr, qp_attr_mask))) {
                err = "ib_modify_qp rtr->rts";
                goto error;
        }

        if (verify_qp_state(td->qp, IB_QPS_RTS)) {
                err = "qp not in IB_QPS_RTS";
                goto error;
        }
	return 0;
error:
	printk(KERN_ERR "%s : %s failed! err %d \n", __func__, err, ret);
	return 1;
}

static void test_post_send_thread_deinit(struct thread_data *td)
{
	int ret;

	if (td->qp)
		if ((ret = ib_destroy_qp(td->qp))) {
			printk(KERN_ERR "%s : ib_destroy_qp failed! err %d \n",
				__func__, ret);
		}
	if (td->cq)
		if ((ret = ib_destroy_cq(td->cq))) {
			printk(KERN_ERR "%s : ib_destroy_cq failed! err %d \n",
				__func__, ret);
		}
	if (td->send_buf_dma)
		ib_dma_unmap_single(td->ibdev, td->send_buf_dma, BUF_LEN,
			DMA_TO_DEVICE);
	if (td->recv_buf_dma)
		ib_dma_unmap_single(td->ibdev, td->recv_buf_dma, BUF_LEN,
			DMA_FROM_DEVICE);
	if (td->mr)
		if ((ret = ib_dereg_mr(td->mr))) {
			printk(KERN_ERR "%s : ib_dereg_mr failed! err %d \n",
				__func__, ret);
		}
	if (td->pd)
		if ((ret = ib_dealloc_pd(td->pd))) {
			printk(KERN_ERR "%s : ib_dealloc_pd failed! err %d \n",
				__func__, ret);
		}
}

int test_post_send_cli_thread(void *data)
{
	char *err = "none";
	struct thread_data *td = (struct thread_data *)data;
	struct thread_pair *tp = container_of(td, struct thread_pair, cli);
	struct ib_send_wr wr;
	struct ib_send_wr *bad_wr;
	struct ib_sge sge;
	int ret = 0;

	if (test_post_send_thread_init(td)) {
		err = "test_post_send_thread_init";
		goto error;
	}
	
	/* send completion to server */
	complete(&td->comp);
	/* wait for srv comp */
	if (wait_for_completion_interruptible(&tp->srv.comp)) {
		err = "wait_for_completion_interruptible";
		goto error_2;
	}
	
	/* server has qpn - continue qp bring up */
	if (test_post_send_thread_init_2(td)) {
                err = "test_post_send_thread_init_2";
                goto error_2;
        }
	
	/* send completion to server */
	complete(&td->comp);
	/* wait for srv comp */
	if (wait_for_completion_interruptible(&tp->srv.comp)) {
		err = "wait_for_completion_interruptible";
		goto error_3;
	}

	/* server & client qps are fully set up */

	memset(td->send_buf, 0, BUF_LEN);
	strncpy(td->send_buf, "client msg", BUF_LEN-1);

	/* post send msg to server */
	memset(&sge, 0, sizeof sge);
	//sge.addr = td->send_buf_dma;
	sge.addr = (u64)td->send_buf;
	sge.length = BUF_LEN;
	sge.lkey = td->mr->lkey;

	memset(&wr, 0, sizeof wr);
	wr.sg_list = &sge;
	wr.num_sge = 1;
	wr.opcode = IB_WR_SEND;
	//wr.send_flags = 0; // IB_SEND_INLINE
	wr.send_flags = IB_SEND_INLINE;

	if ((ret = ib_post_send(td->qp, &wr, &bad_wr))) {
		err = "ib_post_send";
		goto error_3;
	}
	
	/* poll cq - send */
	if (test_poll_cq(td->cq, 3)) {
		err = "test_poll_cq";
		goto error_3;
	}

	test_post_send_thread_deinit(td);
	printk(KERN_ERR "%s : thread exit \n", __func__);
	/* send completion to server */
	complete(&td->comp);
	return 0;
error:
	/* send comp to srv */
	complete(&td->comp);
error_2:
	/* send comp to srv */
	complete(&td->comp);
error_3:
	test_post_send_thread_deinit(td);
	printk(KERN_ERR "%s : %s failed! err %d \n", __func__, err, ret);
	printk(KERN_ERR "%s : thread exit \n", __func__);
	/* send comp to srv */
	complete(&td->comp);
	return 1;
}

int test_post_send_srv_thread(void *data)
{
	char *err = "none";
	struct thread_data *td = (struct thread_data *)data;
	struct thread_pair *tp = container_of(td, struct thread_pair, srv);
	
	if (test_post_send_thread_init(td)) {
		err = "test_post_send_thread_init";
		goto error;
	}

	/* send comp to cli */
	complete(&td->comp);
	/* wait for cli comp */
	if (wait_for_completion_interruptible(&tp->cli.comp)) {
		printk(KERN_ERR "%s : failed to wait for cli comp! \n", __func__);
		return 1;
	}
	
	/* client has qpn - continue qp bring up */
	if (test_post_send_thread_init_2(td)) {
		err = "test_post_send_thread_init_2";
		goto error_2;
	}

	memset(td->recv_buf, 0, BUF_LEN);

	/* send comp to cli */
	complete(&td->comp);
	/* wait for cli comp */
	if (wait_for_completion_interruptible(&tp->cli.comp)) {
		printk(KERN_ERR "%s : failed to wait for cli comp! \n", __func__);
		return 1;
	}
	
	/* client & server are fully set up */

	/* poll cq - recv */
	if (test_poll_cq(td->cq, 5)) {
		err = "test_poll_cq";
		goto error_2;
	}

	/* got cli msg */
	printk(KERN_ERR "%s : client msg: %s \n", __func__, td->recv_buf);
	
	/* wait for cli comp */
	if (wait_for_completion_interruptible(&tp->cli.comp)) {
		printk(KERN_ERR "%s : failed to wait for cli comp! \n", __func__);
		return 1;
	}
	test_post_send_thread_deinit(td);
	printk(KERN_ERR "%s : thread exit \n", __func__);
	/* send comp to main thread */
	complete(&tp->comp);
	return 0;
error:
	/* send completion #1 to cli */
	complete(&td->comp);
error_2:
	/* send completion #2 to cli */
	complete(&td->comp);
	/* wait for cli comp */
	if (wait_for_completion_interruptible(&tp->cli.comp)) {
		printk(KERN_ERR "%s : failed to wait for cli comp! \n", __func__);
		return 1;
	}
	test_post_send_thread_deinit(td);
	printk(KERN_ERR "%s : %s failed! \n", __func__, err);
	printk(KERN_ERR "%s : thread exit \n", __func__);
	/* send comp to main thread */
	complete(&tp->comp);
	return 1;
}

/*
 * kvl specific function & test entry point
 * */
int test_post_send(struct ib_device *ibdev, struct thread_pair *tp,
	struct completion *test_comp)
{
	char *err = "none";
	if (init_thread_pair(tp, ibdev)) {
		err = "init_thread_pairs";
		goto error;
	}
	tp->srv.task = kthread_run(test_post_send_srv_thread, &tp->srv,
		"test_post_send_srv");
	if (IS_ERR(tp->srv.task)) {
		err = "create srv thread";
		goto error;
	}
	tp->cli.task = kthread_run(test_post_send_cli_thread, &tp->cli,
		"test_post_send_srv");
	if (IS_ERR(tp->srv.task)) {
		err = "create cli thread";
		goto error;
	}
	/* wait for server thread to complete. server thread
	wait for client thread so we are done here */
	if (wait_for_completion_interruptible(&tp->comp)) {
		err = "server completion";
		goto error;
	}
	deinit_thread_pair(tp);
	printk(KERN_ERR "%s : test complete \n", __func__);
	complete(test_comp);
	return 0;
error:
	printk(KERN_ERR "%s : %s failed! \n", __func__, err);
	printk(KERN_ERR "%s : main thread exit \n", __func__);
	return 1;	
}

