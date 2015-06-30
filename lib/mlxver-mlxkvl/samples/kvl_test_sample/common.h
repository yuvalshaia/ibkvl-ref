#ifndef __KVL_SAMPLE_COMMON__ 
#define __KVL_SAMPLE_COMMON__ 

#define TEST_MODULE_NAME "mlxkvl-sample"
#define PROCFS_NAME 	"mlxkvl"


#define print_info(fmt, args...) printk( KERN_INFO  "[%s] : " fmt,TEST_MODULE_NAME, ## args)
#define print_warn(fmt, args...) printk( KERN_WARNING "[%s] warning : " fmt,TEST_MODULE_NAME, ## args)
#define print_err(fmt, args...) printk( KERN_ERR "[%s] error : " fmt,TEST_MODULE_NAME, ## args)

struct user_st{
    int integ;
    char* str;
};

#endif

