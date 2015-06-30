/*
 * kvl_common.h -- definitions 
 *
 * Copyright (C) 2011 Saeedm <saeedm@mellanox.com>
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  
 * No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 */
 
#ifndef _kvl_common_h_
#define _kvl_common_h_

#define IN
#define OUT

#define KVL_MODULE_NAME "mlxkvl"
#define PROCFS_NAME 	"mlxkvl"

/*
 * Macros to help debugging
 */
 
#define print_info(fmt, args...) printk( KERN_INFO  "[%s] : " fmt,KVL_MODULE_NAME, ## args)
#define print_warn(fmt, args...) printk( KERN_WARNING "[%s] warning : " fmt,KVL_MODULE_NAME, ## args)
#define print_err(fmt, args...) printk( KERN_ERR "[%s] error : " fmt,KVL_MODULE_NAME, ## args)

//#undef GMOD_DEBUG             /* undef it, just in case */
#ifdef GMOD_DEBUG
#	define print_dbg(fmt, args...) printk( KERN_DEBUG "[%s] debug %s:%d : " fmt,KVL_MODULE_NAME,__FILE__,__LINE__, ## args)
#else
#  define print_dbg(fmt, args...) /* not debugging: nothing */
#endif

#endif

