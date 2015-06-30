/*
 * Copyright (c) 2011 Mellanox Technologies. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * $Id: mlxkvl-main.c 3443 2011-08-21 14:00:11Z saeedm $ 
 * 
 */
 
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/slab.h> /* kmalloc() */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/proc_fs.h>	/* Necessary because we use proc fs */
#include <asm/uaccess.h>	/* for copy_*_user */

#include <linux/list.h>

#include "kvl_common.h"
#include "kvl.h"
#include "vl_os.h"

MODULE_AUTHOR("Saeed Mahameed - Mellanox Technologies");
MODULE_DESCRIPTION("Testing module & VL library for linux kernel : for more info 'man mlxkvl'");
MODULE_LICENSE ("GPL");

#define IS_EMPTY_STR(str) (!strlen(str) || !strcmp(str, "\0") || !strcmp(str, "") || !strcmp(str	, "\n"))

//procfs root directory
static struct proc_dir_entry *gmod_proc_root;

// registered operations list
LIST_HEAD(kvl_op_list);
// list mutex
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
DECLARE_MUTEX(kvl_list_mutex);
#else
DEFINE_SEMAPHORE(kvl_list_mutex);
#endif


// print info about current registerd operation
int info_run(void * kvldata, void *data,void* userbuff,unsigned int len)
{
	//struct kvl_op* op = (struct kvl_op*)kvldata;
	struct list_head *cursor;
	int res;
	print_info("KVL INFO :- Showing information about all the loaded KVL operations\n");
	res = down_interruptible(&kvl_list_mutex);
	if (res) {
		print_info( "aborting operation ... (killed by user)\n");
		return -1;
	}
	list_for_each(cursor, &kvl_op_list) {
		struct kvl_op* op=(struct kvl_op*)cursor;
		if (op->user_app) 
			print_info( "[%s]\t/proc/%s/%s [ user_app = %s ]: %s \n",op->module,PROCFS_NAME ,op->name,op->user_app ,op->descr);
		else
			print_info( "[%s]\t/proc/%s/%s : %s \n",op->module,PROCFS_NAME ,op->name, op->descr);
	}
	up(&kvl_list_mutex);
	return 0;
}


/** 
 * This function is called when the /proc/mlxkvl/... file is 
 * read 
 *
 */
int procfile_read(char *buffer,char **buffer_location,off_t offset, int buffer_length, int *eof, void *data)
{
	int ret = -1;
	struct kvl_op* op = (struct kvl_op*)data;	
	print_info( "procfile_read (/proc/%s/%s) called\n", PROCFS_NAME,op->name);	
	ret = strlen(op->descr);
	memcpy(buffer,op->descr, ret);
	return ret;
}

int kvl_parse_args(char **argv,struct kvl_op* op)
{
	int rc = 0;
	char *arg = NULL;
	char* delems = " =\n";
	struct list_head *cursor;
	
	if ((*argv)[strlen(*argv)-1] == '\n') {
		(*argv)[strlen(*argv)-1] = '\0';
	}

	//print_info("Calling kvl_parse_args whith %s", *argv);
	list_for_each(cursor, &op->param_list.list) {
		struct argument* arg=(struct argument*)cursor;
		arg->value = NULL;
	}

	arg = strsep( argv, delems );
	while(arg != NULL ) {
		if (IS_EMPTY_STR(arg)) {
			arg = strsep( argv,delems);
			continue;
		}
		if (!strcmp(arg, "--help")) {
			print_info("Info :-\n");
			print_info("\t%s : %s\n",op->name,op->descr);
			print_info("Params : \n");
			list_for_each(cursor, &op->param_list.list) {
				struct argument* param=(struct argument*)cursor;
				switch(param->type) {
				case INT:
					print_info("\t%s\t : %s . default value : %d\n",param->name,param->descreption,param->intdefval);
					break;
				case STR:
					print_info("\t%s\t : %s . default value : %s\n",param->name,param->descreption,param->strdefval);
					break;
				default:
					print_info("\t%s\t : %s\n",param->name,param->descreption);
				}
			}
			return -1;
		}
		list_for_each(cursor, &op->param_list.list) {
			struct argument* param=(struct argument*)cursor;
			if (!strcmp(param->name, arg)) {
				arg = strsep( argv, delems );
				print_info("found param %s = %s \n",param->name,arg);
				param->value = arg;
			}
		}
		arg = strsep( argv, delems );
    }
   
	return rc;
}

/** 
 * This function is called when the /proc/mlxkvl/... file is 
 * written
 */
int procfile_write(struct file *file, const char *buffer, unsigned long count,void *data)
{
	struct kvl_op* op = (struct kvl_op*)data;
	char* input_buffer = NULL; 
	int input_size = 0;
	int ret;
	//char * buff=input_buffer; 

	print_info( "procfile_write (/proc/%s/%s) [%ld]\n", PROCFS_NAME,op->name,count);	
	/* write data to the buffer */
	input_size = count > MAX_INPUT_SZ ? MAX_INPUT_SZ : count;
	input_buffer = (char *)kmalloc(input_size+1,GFP_KERNEL);
	if (!input_buffer) 
		return -ENOMEM;

	if ( copy_from_user(input_buffer, buffer, input_size) ) {
		print_err("[%s] Failed to copy user input buffer\n",op->module);
		kfree(input_buffer);
		return -EFAULT;
	}
	if (!op->user_app){
		input_buffer[input_size] = '\0';
		print_info("executing using sysfs command parse: \n");
	} else {
		print_info("executing using : %s\n",op->user_app);
	}

	//print_info("/proc/%s/%s %s", PROCFS_NAME,op->name,input_buffer);
	print_info("***************** Executing %s ***************** \n",op->name);
	if (op->op_run) {
		if (input_size && (op->user_app == NULL)) {
			char *argv = input_buffer;
			print_info("Params : %s\n",input_buffer);
			ret = kvl_parse_args(&argv,op);
			if (ret != 0) {
				ret = -EFAULT;
				print_err("[%s] [%s] Failed to parse arguments\n",op->module,op->name);
				goto exit;
			}
		}
		ret = op->op_run(op,op->data,input_buffer,input_size);
	} else {
		print_err("[%s] [%s] run function pointer is set to null\n",op->module,op->name);
		ret = -EINVAL;
	}
exit :
	print_info("***************** Done %s rc=[%d] ************* \n",op->name,ret);

	kfree(input_buffer);
	if (ret != 0) 
		ret = -EFAULT;
	else 
		ret = count;
	return ret;
};



int register_kvl_op(struct kvl_op* operation){
	struct proc_dir_entry *new_proc_file;
	if (operation == NULL){
		print_warn(" Trying to register a NULL operation");
		return -1;
	}
	
	print_info( "[%s] registering operation : %s\n" ,operation->module ,operation->name);
	new_proc_file = create_proc_entry(operation->name, 0777, gmod_proc_root);
	if (!new_proc_file) 
	{
		print_err( "Error: Could not initialize /proc/%s/%s\n",PROCFS_NAME,operation->name);			
		return -1;
	}
	print_info( "[%s] created procfs : /proc/%s/%s\n" ,operation->module,PROCFS_NAME, operation->name);
	new_proc_file->data = operation;
	new_proc_file->read_proc  = procfile_read;
	new_proc_file->write_proc = procfile_write;
	new_proc_file->mode       = S_IFREG | S_IRUGO;				
	//add the operation to the list
	down(&kvl_list_mutex);
	list_add_tail(&operation->list,&kvl_op_list);
	up(&kvl_list_mutex);
	return 0;
}
EXPORT_SYMBOL(register_kvl_op);

int unregister_kvl_op(struct kvl_op* operation){
	if (operation == NULL){
		print_warn(" Trying to unregister a NULL operation");
		return -1;
	}
	print_info( "[%s] unregistering operation : %s\n" ,operation->module, operation->name);
	remove_proc_entry(operation->name, gmod_proc_root);
	print_info( "[%s] destroyed procfs : /proc/%s/%s\n" ,operation->module,PROCFS_NAME, operation->name);
	//rmove the operation from the list
	down(&kvl_list_mutex);
	list_del_init(&operation->list);
	up(&kvl_list_mutex);
	return 0;
}
EXPORT_SYMBOL(unregister_kvl_op);

struct kvl_op* kvl_info;

/*****************************************************************************
* Function: KVL_driver_init
*****************************************************************************/
int KVL_driver_init(void)
{
//	int ret = 0;
	print_info("Loading module ...\n");
#ifdef GMOD_DEBUG
	print_warn("running in deug mode ...\n");
#endif
	/* create the /proc file */

	gmod_proc_root = proc_mkdir(PROCFS_NAME,NULL);
	if (gmod_proc_root == NULL) {
			print_err( "Could not initialize /proc/%s\n",PROCFS_NAME);
			return -ENOMEM;
	}
	print_info("created /proc/%s\n",PROCFS_NAME);
	kvl_info = create_kvlop("kvlinfo","print kvl info","mlxkvl",info_run,NULL,NULL);
	print_info("Module Loaded\n");
    return 0;
}

/*****************************************************************************
* Function: KVL_driver_exit
*****************************************************************************/
void KVL_driver_exit(void)
{
	print_info("Unloading module ...\n");
	//unregister_kvl_op(&info_op);
	destroy_kvlop(kvl_info);
	
	remove_proc_entry(PROCFS_NAME, NULL);
	print_info( "/proc/%s removed\n", PROCFS_NAME);
    print_info("Module unloaded\n");
}

module_init(KVL_driver_init);
module_exit(KVL_driver_exit);

