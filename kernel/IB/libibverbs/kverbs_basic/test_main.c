#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>

// mlxkvlincludes
#include <vl.h>
#include <vl_os.h>
#include <kvl.h>

#include "my_types.h"
#include "func_headers.h"

#define DEV_NAME_SZ (64)

MODULE_LICENSE("GPL");                    
MODULE_AUTHOR("Saeed Mahameed , saeedm@mellanox.com");            
MODULE_DESCRIPTION("kernel ib verbs basic test");

#define MODULE_NAME "kverbs_basic"
//module parameters example
static char ibdevice[DEV_NAME_SZ]="mlx4_0";
module_param_string(device, ibdevice, DEV_NAME_SZ, 0644);
MODULE_PARM_DESC(device, "ib device to use , default = 'mlx4_0' ");

struct ib_client test_client;

struct resources res;

/*****************************************************************************
* Function: test_init_resources
*****************************************************************************/
void test_init_resources(void)
{
	memset(&res, 0, sizeof(res));
}

int kvl_test_CQ(void* kvldata, void *data,void* userbuff,unsigned int len){
	struct kvl_op* op = (struct kvl_op*)kvldata;
	struct config_t config;
	int rc;
	strcpy(config.hca_id,ibdevice);
	config.test_num = get_param_intval(op,"test");
	config.leave_open = get_param_intval(op,"leaveres");
	VL_DATA_TRACE(("#############[Test CQ]################"));
	VL_DATA_TRACE(("\t\tibdev = %s",ibdevice));
	VL_DATA_TRACE(("\t\ttest  = %d",config.test_num));
	VL_DATA_TRACE(("\t\tleave resource open   = %d",config.leave_open));
	VL_srand(config.seed, &res.rand_g);
	rc = test_cq(&config, &res, &test_client);
	return rc;
}

int kvl_test_QP(void* kvldata, void *data,void* userbuff,unsigned int len){
	struct kvl_op* op = (struct kvl_op*)kvldata;
	struct config_t config;
	int rc;
	strcpy(config.hca_id,ibdevice);
	config.test_num = get_param_intval(op,"test");
	config.leave_open = get_param_intval(op,"leaveres");
	VL_DATA_TRACE(("#############[Test QP]################"));
	VL_DATA_TRACE(("\t\tibdev = %s",ibdevice));
	VL_DATA_TRACE(("\t\ttest  = %d",config.test_num));
	VL_DATA_TRACE(("\t\tleave resource open   = %d",config.leave_open));
	VL_srand(config.seed, &res.rand_g);
	rc = test_qp(&config, &res, &test_client);
	return rc;
}

int kvl_test_HCA(void* kvldata, void *data,void* userbuff,unsigned int len){
	struct kvl_op* op = (struct kvl_op*)kvldata;
	struct config_t config;
	int rc;
	strcpy(config.hca_id,ibdevice);
	config.test_num = get_param_intval(op,"test");
	config.leave_open = get_param_intval(op,"leaveres");
	VL_DATA_TRACE(("#############[Test HCA]################"));
	VL_DATA_TRACE(("\t\tibdev = %s",ibdevice));
	VL_DATA_TRACE(("\t\ttest  = %d",config.test_num));
	VL_DATA_TRACE(("\t\tleave resource open   = %d",config.leave_open));
	VL_srand(config.seed, &res.rand_g);
	rc = test_hca(&config, &res, &test_client);
	return rc;
}

int kvl_test_AV(void* kvldata, void *data,void* userbuff,unsigned int len){
	struct kvl_op* op = (struct kvl_op*)kvldata;
	struct config_t config;
	int rc;
	strcpy(config.hca_id,ibdevice);
	config.test_num = get_param_intval(op,"test");
	config.leave_open = get_param_intval(op,"leaveres");
	VL_DATA_TRACE(("#############[Test AV]################"));
	VL_DATA_TRACE(("\t\tibdev = %s",ibdevice));
	VL_DATA_TRACE(("\t\ttest  = %d",config.test_num));
	VL_DATA_TRACE(("\t\tleave resource open   = %d",config.leave_open));
	VL_srand(config.seed, &res.rand_g);
	rc = test_av(&config, &res, &test_client);
	return rc;
}

int kvl_test_MR(void* kvldata, void *data,void* userbuff,unsigned int len){
	struct kvl_op* op = (struct kvl_op*)kvldata;
	struct config_t config;
	int rc;
	strcpy(config.hca_id,ibdevice);
	config.test_num = get_param_intval(op,"test");
	config.leave_open = get_param_intval(op,"leaveres");
	VL_DATA_TRACE(("#############[Test MR]################"));
	VL_DATA_TRACE(("\t\tibdev = %s",ibdevice));
	VL_DATA_TRACE(("\t\ttest  = %d",config.test_num));
	VL_DATA_TRACE(("\t\tleave resource open   = %d",config.leave_open));
	VL_srand(config.seed, &res.rand_g);
	rc = test_mr(&config, &res, &test_client);
	return rc;
}

int kvl_test_PD(void* kvldata, void *data,void* userbuff,unsigned int len){
	struct kvl_op* op = (struct kvl_op*)kvldata;
	struct config_t config;
	int rc;
	strcpy(config.hca_id,ibdevice);
	config.test_num = get_param_intval(op,"test");
	config.leave_open = get_param_intval(op,"leaveres");
	VL_DATA_TRACE(("#############[Test PD]################"));
	VL_DATA_TRACE(("\t\tibdev = %s",ibdevice));
	VL_DATA_TRACE(("\t\ttest  = %d",config.test_num));
	VL_DATA_TRACE(("\t\tleave resource open   = %d",config.leave_open));
	VL_srand(config.seed, &res.rand_g);
	rc = test_pd(&config, &res, &test_client);

	return rc;
}

int kvl_test_SRQ(void* kvldata, void *data,void* userbuff,unsigned int len){
	struct kvl_op* op = (struct kvl_op*)kvldata;
	struct config_t config;
	int rc;
	strcpy(config.hca_id,ibdevice);
	config.test_num = get_param_intval(op,"test");
	config.leave_open = get_param_intval(op,"leaveres");
	VL_DATA_TRACE(("#############[Test SRQ]################"));
	VL_DATA_TRACE(("\t\tibdev = %s",ibdevice));
	VL_DATA_TRACE(("\t\ttest  = %d",config.test_num));
	VL_DATA_TRACE(("\t\tleave resource open   = %d",config.leave_open));
	VL_srand(config.seed, &res.rand_g);
	rc = test_srq(&config, &res, &test_client);

	return rc;
}

int kvl_test_POLLPOST(void* kvldata, void *data,void* userbuff,unsigned int len){
	struct kvl_op* op = (struct kvl_op*)kvldata;
	struct config_t config;
	int rc;
	strcpy(config.hca_id,ibdevice);
	config.test_num = get_param_intval(op,"test");
	config.leave_open = get_param_intval(op,"leaveres");
	VL_DATA_TRACE(("#############[Test POLLPOST]################"));
	VL_DATA_TRACE(("\t\tibdev = %s",ibdevice));
	VL_DATA_TRACE(("\t\ttest  = %d",config.test_num));
	VL_DATA_TRACE(("\t\tleave resource open   = %d",config.leave_open));
	VL_srand(config.seed, &res.rand_g);
	rc = test_pollpost(&config, &res, &test_client);

	return rc;
}


int kvl_test_multicast(void* kvldata, void *data,void* userbuff,unsigned int len){
	struct kvl_op* op = (struct kvl_op*)kvldata;
	struct config_t config;
	int rc;
	strcpy(config.hca_id,ibdevice);
	config.test_num = get_param_intval(op,"test");
	config.leave_open = get_param_intval(op,"leaveres");
	VL_DATA_TRACE(("#############[Test multicast]################"));
	VL_DATA_TRACE(("\t\tibdev = %s",ibdevice));
	VL_DATA_TRACE(("\t\ttest  = %d",config.test_num));
	VL_DATA_TRACE(("\t\tleave resource open   = %d",config.leave_open));
	VL_srand(config.seed, &res.rand_g);
	rc = test_multicast(&config, &res, &test_client);

	return rc;
}

struct kvl_op* test_cq_op = NULL;
struct kvl_op* test_qp_op = NULL;
struct kvl_op* test_hca_op = NULL;
struct kvl_op* test_mr_op = NULL;
struct kvl_op* test_pd_op = NULL;
struct kvl_op* test_av_op = NULL;
struct kvl_op* test_srq_op = NULL;
struct kvl_op* test_mcast_op = NULL;
struct kvl_op* test_pollpost_op = NULL;

#define INIT_TEST(test_op,test_func,test_name,test_info,tests_descr)		\
	test_op = create_kvlop(test_name,test_info,MODULE_NAME,test_func,NULL,NULL); \
	add_int_param(test_op,"test",tests_descr,1);							\
	add_int_param(test_op,"leaveres","leave resources open",0)

/*****************************************************************************
* Function: query_add_one
*****************************************************************************/
static void test_add_one(
	IN	struct ib_device *device)
{

	VL_MISC_TRACE1(("%s enter query_add_one",MODULE_NAME ));
	if (strcmp(device->name, ibdevice) != 0) {
		printk("[%s] ERROR: add device name %s doesn't match.\n", MODULE_NAME, device->name);
		goto exit;
	}

	res.name = ibdevice;
	res.device = device;

	if (ib_query_device(device, &res.device_attr)) {
		printk("[%s] ERROR: ib_query_device failed\n", MODULE_NAME);
		goto exit;
	}

	res.dma_local_lkey = !!(res.device_attr.device_cap_flags & IB_DEVICE_LOCAL_DMA_LKEY);

	if (ib_query_port(device, VL_range(&res.rand_g, 1, device->phys_port_cnt), &res.port_attr)) {
		printk("[%s] ERROR: ib_query_port failed\n", MODULE_NAME);
		goto exit;
	}
	//ibdriver found , test can be created .
	printk("[%s] add success to ib device: %s", MODULE_NAME,device->name);
	INIT_TEST(test_cq_op,kvl_test_CQ,"test_CQ",CQ_INFO,"test num to run");
	INIT_TEST(test_qp_op,kvl_test_QP,"test_QP",QP_INFO,"test num to run");
	INIT_TEST(test_hca_op,kvl_test_HCA,"test_HCA",HCA_INFO,"test num to run");
	INIT_TEST(test_mr_op,kvl_test_MR,"test_MR",MR_INFO,"test num to run");
	INIT_TEST(test_pd_op,kvl_test_PD,"test_PD",PD_INFO,"test num to run");
	INIT_TEST(test_av_op,kvl_test_AV,"test_AV",AV_INFO,"test num to run");
	INIT_TEST(test_srq_op,kvl_test_SRQ,"test_SRQ",SRQ_INFO,"test num to run");
	INIT_TEST(test_mcast_op,kvl_test_multicast,"test_MCAST",MCAST_INFO,"test num to run");
	INIT_TEST(test_pollpost_op,kvl_test_POLLPOST,"test_POLLPOST",POLLPOST_INFO,"test num to run");

exit:
	VL_MISC_TRACE1(("%s exit query_add_one", MODULE_NAME));
}

static void test_remove_one(
	IN	struct ib_device *device)
{
	if (strcmp(device->name, ibdevice) != 0) {
		printk("[%s] ERROR: remove device name %s doesn't match.\n", MODULE_NAME, device->name);
		return ;
	}
	printk("[%s] remove success , ib device: %s", MODULE_NAME,device->name);
	destroy_kvlop(test_cq_op);
	destroy_kvlop(test_qp_op);
	destroy_kvlop(test_hca_op);
	destroy_kvlop(test_mr_op);
	destroy_kvlop(test_av_op);
	destroy_kvlop(test_srq_op);
	destroy_kvlop(test_pd_op);
	destroy_kvlop(test_mcast_op);
	destroy_kvlop(test_pollpost_op);
}

static int init_kverbs_basic_test(void)
{
	printk(KERN_ALERT "LOADING [%s] test AKA: 'gen2_basic_kernel' - with ibdevice = %s ...\n",MODULE_NAME,ibdevice);
	test_client.name   = MODULE_NAME;
    test_client.add    = test_add_one;
    test_client.remove = test_remove_one;

	test_init_resources();

	if (ib_register_client(&test_client)) {								
		  printk("[%s] couldn't register IB client\n", MODULE_NAME);		
		  return -ENODEV;						
	}				
	printk(KERN_ALERT "%s test was loaded\n",MODULE_NAME );
	return 0;
}


static void cleanup_kverbs_basic_test(void)
{
	printk(KERN_ALERT "UNLOADING [%s] test ...\n",MODULE_NAME);
	ib_unregister_client(&test_client);
	printk(KERN_ALERT "%s test was unloaded\n",MODULE_NAME);
}


module_init(init_kverbs_basic_test);
module_exit(cleanup_kverbs_basic_test);



