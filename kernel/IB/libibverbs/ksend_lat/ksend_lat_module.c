#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/fs.h>       /* for register char device */
#include <asm/uaccess.h>    /* for copy_from_user */

#include <kvl.h>

#include "ksend_lat.h"
#include "ksend_lat_kernel.h"


MODULE_AUTHOR ("Liran Liss / Saeed Mahameed - Mellanox Technologies LTD");
MODULE_DESCRIPTION ("Kernel latency performance test");
MODULE_SUPPORTED_DEVICE ("Mellanox HCAs");
MODULE_LICENSE ("GPL");
#define MODULE_NAME MOD_NAME

#define USER_APP "ib_ksend_lat"

//extern struct config_t config;

int kvl_test_ksendlat(void* kvldata, void *data,void* userbuff,unsigned int len){
//	struct kvl_op* op = (struct kvl_op*)kvldata;
	params_t* ksl_params;
	//params_t *usr_ksl_params_ptr = (params_t*) buff;
	int rc = 0;

	//if (copy_from_user(&ksl_params, usr_ksl_params_ptr, sizeof(ksl_params)))
	//return -EFAULT;
	if (len != sizeof(params_t)) {
		printk(KSL_ERR_FMT"len [%d] != [%ld] sizeof(params_t)\n ",len,sizeof(params_t));
		return -EINVAL;
	}
	ksl_params = (params_t*)userbuff;
	switch (ksl_params->cmd) {
	case INIT:
		printk(KSL_MSG_FMT "INIT called\n");
		rc = ksl_init(ksl_params);
		break;

	case CONNECT:
		printk(KSL_MSG_FMT "CONNECT called\n");
		rc = ksl_connect_qps(ksl_params);
		break;

	case TEST_SEND_LAT:
		printk(KSL_MSG_FMT "TEST_SEND_LAT called\n");
		rc = ksl_test_send_lat(ksl_params);
		break;

	case CLEANUP:
		printk(KSL_MSG_FMT "CLEANUP called\n");
		ksl_cleanup();
		break;

	default:
		printk(KSL_ERR_FMT "Invalid command\n");
		rc = - EINVAL;
	}

	return rc;
}

struct kvl_op* test_ksendlat_op = NULL;


static int init_ksendlat_test(void)
{
	printk(KERN_ALERT "LOADING [%s] test ...\n",MODULE_NAME);	
	test_ksendlat_op = create_kvlop(MODULE_NAME,"Kernel post send latency test",MODULE_NAME,kvl_test_ksendlat,NULL,USER_APP); 	
	printk(KERN_ALERT "%s test was loaded\n",MODULE_NAME );
	return 0;
}

static void cleanup_ksendlat_test(void)
{
	printk(KERN_ALERT "UNLOADING [%s] test ...\n",MODULE_NAME);
	destroy_kvlop(test_ksendlat_op);
	printk(KERN_ALERT "%s test was unloaded\n",MODULE_NAME);
}


module_init(init_ksendlat_test);
module_exit(cleanup_ksendlat_test);





#if 0
//
// Globals
// 
static int VL_driver_generic_major;			/* device major number */
static const char *VL_driver_generic_name = MOD_NAME;	/* driver name */


//
// File operations
// 

static int VL_kdriver_generic_open(struct inode *inode, struct file *file)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	MOD_INC_USE_COUNT;
#endif
	printk(KSL_MSG_FMT "open called\n");

	return 0;
}


static int VL_kdriver_generic_release(struct inode *inode, struct file *file)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	MOD_DEC_USE_COUNT;
#endif
	printk(KSL_MSG_FMT "release called\n");

	ksl_cleanup(); // just in case...
	return 0;
}


ssize_t VL_kdriver_generic_write (
		struct file * file,
		const char *buff,
		size_t count,
		loff_t * offp)
{
	params_t ksl_params;
	params_t *usr_ksl_params_ptr = (params_t*) buff;
	int rc = 0;

	if (copy_from_user(&ksl_params, usr_ksl_params_ptr, sizeof(ksl_params)))
		return -EFAULT;
	switch (ksl_params.cmd) {
	case INIT:
		printk(KSL_MSG_FMT "INIT called\n");
		rc = ksl_init(&ksl_params, usr_ksl_params_ptr);
		break;

	case CONNECT:
		printk(KSL_MSG_FMT "CONNECT called\n");
		rc = ksl_connect_qps(&ksl_params);
		break;

	case TEST_SEND_LAT:
		printk(KSL_MSG_FMT "TEST_SEND_LAT called\n");
		rc = ksl_test_send_lat(&ksl_params);
		break;

	case CLEANUP:
		printk(KSL_MSG_FMT "CLEANUP called\n");
		ksl_cleanup();
		break;

	default:
		printk(KSL_ERR_FMT "Invalid command\n");
		rc = - EINVAL;
	}

	if (rc >= 0) {
		// a successful, fully completed write returns the number of bytes that were "written"...
		rc = count;
	}
	return rc;
}


ssize_t VL_kdriver_generic_read (
		struct file * file,
		char *buff,
		size_t count,
		loff_t * offp)
{
	return 0;
}


static int VL_kdriver_generic_ioctl (
		struct inode *inode,
		struct file *file,
		unsigned int i,
		unsigned long l)
{
	return 0;
}


//
// Driver initialization
//

static struct file_operations VL_driver_fops = {
	.open = VL_kdriver_generic_open,
	.release = VL_kdriver_generic_release,
	.write = VL_kdriver_generic_write,
	.read = VL_kdriver_generic_read,
	.ioctl = VL_kdriver_generic_ioctl,
	.owner = THIS_MODULE,
};


int VL_kdriver_generic_init(void)
{
	VL_driver_generic_major = register_chrdev (0, VL_driver_generic_name,
						   &VL_driver_fops);
	if (VL_driver_generic_major < 0) {
		printk ("Error, failed to register char device %s\n",
			VL_driver_generic_name);
		return -1;
	}

	printk (KERN_ALERT
		"%s:-D- %s(%d) Module %s registered and loaded - major %d\n",
		__FILE__, __FUNCTION__, __LINE__, VL_driver_generic_name,
		VL_driver_generic_major);
	ksl_init_resources();
	return 0;
}




void VL_kdriver_generic_exit(void)
{
	int rc = 0;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)
	rc = 
#endif
	unregister_chrdev (VL_driver_generic_major, VL_driver_generic_name);
	if (rc) {
		printk
		("Error, failed to unregister char device %s with major number: %u\n",
		 VL_driver_generic_name, VL_driver_generic_major);
	}

	printk (KERN_ALERT "-D- %s:\t%s(%d) Module Unloaded - major %d\n",
		__FILE__, __FUNCTION__, __LINE__, VL_driver_generic_major);
	return;
}


module_init (VL_kdriver_generic_init);
module_exit (VL_kdriver_generic_exit);
#endif
