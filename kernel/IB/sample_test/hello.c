#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

// mlxkvlincludes
#include <vl.h>
#include <vl_os.h>
#include <kvl.h>

#define DRIVER_AUTHOR "Saeed"
#define DRIVER_DESC   "a sample test driver"

#ifndef MODULE_NAME
#define MODULE_NAME "sample_test"
#endif

int mlnx_command = 0;
module_param_named(command, mlnx_command, int, 0644);
MODULE_PARM_DESC(command, "Enable debug tracing if > 0");


static int init_hello(void);
static void cleanup_hello(void);

int kvl_dummy_test(void* kvldata, void *data,void* userbuff,unsigned int len){
	struct kvl_op* op = (struct kvl_op*)kvldata;
	int rc;
    const char* dev = get_param_strval(op,"dev");
    int port = get_param_intval(op,"ibport");

    rc = 0;
	VL_DATA_TRACE(("Dummy Test dev=%s port=%d ...\n",dev,port));
    return rc;
}

struct kvl_op* dumy_test_op = NULL;

static int init_hello(void)
{
   VL_DATA_TRACE(("init hello kvl ...\n"));
   dumy_test_op = create_kvlop("dummy_test","Dummy Test",MODULE_NAME,kvl_dummy_test,NULL,NULL); 	
   add_str_param(dumy_test_op,"dev","IB device","mlx4_0");		
   add_int_param(dumy_test_op,"ibport","IB port",1);		
   printk(KERN_ALERT "Hello, KVL\n");
   return 0;
}


static void cleanup_hello(void)
{
   destroy_kvlop(dumy_test_op);
   printk(KERN_ALERT "Goodbye, KVL\n");
}


module_init(init_hello);
module_exit(cleanup_hello);


MODULE_LICENSE("GPL");           // Get rid of taint message by declaring code as GPL.

MODULE_AUTHOR(DRIVER_AUTHOR);    // Who wrote this module?
MODULE_DESCRIPTION(DRIVER_DESC); // What does this module do?

/*  This module uses /dev/testdevice.  The MODULE_SUPPORTED_DEVICE macro might be used in
 *   *  the future to help automatic configuration of modules, but is currently unused other
 *    *  than for documentation purposes.
 *     */
MODULE_SUPPORTED_DEVICE("testdevice");
