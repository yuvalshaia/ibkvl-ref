#ifndef _KSEND_LAT_KERNEL_H_
#define _KSEND_LAT_KERNEL_H_


#define KSL_MSG_FMT	KERN_WARNING "[" MOD_NAME "] "
#define KSL_ERR_FMT	KERN_ERR "[" MOD_NAME "] ERROR: "


//#define KSL_PROF
//#define DEBUG

// Currently, the driver ignores the inline flag in sends, but try inlining data
// for small sends anyway...
#define INLINE_THRESHOLD	400

// Although this is a ping pong test, memfree HW gets into limited state when there are a few
// receive WQEs. Any value higher than 10 should be more than enough...
#define RX_DEPTH		20

// Number of reties to poll for send completions after we received the 'pong' response for that send
#define POLL_RETRIES		100

#ifdef DEBUG
#define DBG_MSG(format, ...) printk(format, ## __VA_ARGS__)
#else
#define DBG_MSG(format, ...)
#endif


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
#define init_MUTEX_LOCKED(mutex) sema_init(mutex,0)
#endif

// Prototypes
void ksl_init_resources(void);
int ksl_init(params_t*);
int ksl_connect_qps(params_t*);
int ksl_test_send_lat(params_t*);
void ksl_cleanup(void);


#endif // _KSEND_LAT_KERNEL_H_


