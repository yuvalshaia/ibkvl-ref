#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>

// mlxkvlincludes
#include <vl.h>
#include <vl_os.h>
#include <kvl.h>
#include "types.h"
#include "fmr_test.h"

MODULE_LICENSE("GPL");                    
MODULE_AUTHOR("Saeed Mahameed , saeedm@mellanox.com");            
MODULE_DESCRIPTION("kernel fast memory regions test");

#define MODULE_NAME DRIVER_DEVICE_NAME

int kvl_test_FMR(void* kvldata, void *data,void* userbuff,unsigned int len){
	struct kvl_op* op = (struct kvl_op*)kvldata;
	struct config_t config;
	int rc;
	config.dev_name = get_param_strval(op,"dev");
	config.ib_port = get_param_intval(op,"ibport");
	config.seed = get_param_intval(op,"seed");
	config.num_of_iter = get_param_intval(op,"iter");
	config.num_of_fmr = get_param_intval(op,"fmrs");
	config.bad_flow = get_param_intval(op,"badflow");
	config.num_thread_pairs = get_param_intval(op,"threads");
	config.trace_level = get_param_intval(op,"trace");
	rc = run_fmr_test(&config);
	return rc;
}

struct kvl_op* test_fmr_op = NULL;


static int init_fmr_test(void)
{
	printk(KERN_ALERT "LOADING [%s] test ...\n",MODULE_NAME);	
	test_fmr_op = create_kvlop("fmr_test","FMR TEST",MODULE_NAME,kvl_test_FMR,NULL,NULL); 	
	add_str_param(test_fmr_op,"dev","IB device","mlx4_0");		
	add_int_param(test_fmr_op,"ibport","IB port",1);							
	add_int_param(test_fmr_op,"seed","seed",0);
	add_int_param(test_fmr_op,"iter","iterations",1);
	add_int_param(test_fmr_op,"fmrs","numbers of fmr",1);
	add_int_param(test_fmr_op,"bad_flow","run bad flow ?",0);
	add_int_param(test_fmr_op,"threads","number of thread pairs",2);
	add_int_param(test_fmr_op,"trace","trace level",1);
	printk(KERN_ALERT "%s test was loaded\n",MODULE_NAME );
	return 0;
}

static void cleanup_fmr_test(void)
{
	printk(KERN_ALERT "UNLOADING [%s] test ...\n",MODULE_NAME);
	destroy_kvlop(test_fmr_op);
	printk(KERN_ALERT "%s test was unloaded\n",MODULE_NAME);
}


module_init(init_fmr_test);
module_exit(cleanup_fmr_test);



