#ifndef _PFMR_TEST_H_
#define _PFMR_TEST_H_

// mlxkvlincludes
//#include <vl.h>
//#include <vl_os.h>
#include <rdma/ib_verbs.h>
#include <rdma/ib_fmr_pool.h>
#include <kvl.h>
#include <linux/list.h>


#ifndef MODULE_NAME
#define MODULE_NAME "pfmr_ktest"
#endif

#define DEBUG

struct dma_entry{
	struct list_head list;
	void *buff;
	void *va;
	u64 pa;
	int size;
	int index;
};

struct fmr_entry{
	struct list_head list;
	struct ib_fmr *fmr;
	struct dma_entry *dma_map;
	int index;
};

struct cq_entry{
	struct list_head list;
	struct ib_cq *cq;
	int index;
};

struct qp_entry{
	struct list_head list;
	struct ib_qp *qp;
	int index;
	int send_cq_index;
	int recv_cq_index;
};

struct pfmr_test_ctx {
	struct kvl_op *create_fmr_op;
	struct kvl_op *destroy_fmr_op;
	struct kvl_op *create_dma_op;
	struct kvl_op *destroy_dma_op;
	struct kvl_op *fmr_map_op;
	struct kvl_op *fmr_pool_op;
	struct kvl_op *cq_op;
	struct kvl_op *qp_op;
	struct kvl_op *complete_test_op;
	struct ib_device *ibdev;
	struct ib_pd *pd;
	struct list_head fmr_list;
	struct list_head dma_list;
	struct list_head cq_list;
	struct list_head qp_list;
};

struct pfmr_test_ctx *pfmr_create_test_ctx(struct ib_device *ibdev);
void pfmr_destroy_test_ctx(struct pfmr_test_ctx *test_ctx);

int pfmr_create_fmr(void *kvldata, void *data, void *userbuff,
		    unsigned int len);
int pfmr_destroy_fmr(void *kvldata, void *data, void *userbuff,
		     unsigned int len);
int pfmr_create_dma_buff(void *kvldata, void *data, void *userbuff,
			 unsigned int len);
int pfmr_dma_ops(void *kvldata, void *data, void *userbuff, unsigned int len);
int pfmr_map_dma_fmr(void *kvldata, void *data, void *userbuff,
		     unsigned int len);
int pfmr_create_pool(void *kvldata, void *data, void *userbuff,
		     unsigned int len);
int pfmr_cq_op(void *kvldata, void *data, void *userbuff, unsigned int len);
int pfmr_qp_op(void *kvldata, void *data, void *userbuff, unsigned int len);
int pfmr_complete_test(void *kvldata, void *data, void *userbuff,
		       unsigned int len);

/*
 * Macros to help debugging
*/

#define info(fmt, args...) printk( KERN_INFO  "[%s] " fmt,MODULE_NAME, ## args)
#define warn(fmt, args...) printk( KERN_WARNING "[%s] warning: " fmt,MODULE_NAME, ## args)
#define error(fmt, args...) printk( KERN_ERR "[%s] error: " fmt,MODULE_NAME, ## args)

//#undef GMOD_DEBUG             /* undef it, just in case */
#ifdef DEBUG
#   define debug(fmt, args...) printk( KERN_DEBUG "[%s] debug [%s:%d] : " fmt,MODULE_NAME ,__FUNCTION__,__LINE__, ## args)
#else
#  define debug(fmt, args...) /* not debugging: nothing */
#endif



#endif
