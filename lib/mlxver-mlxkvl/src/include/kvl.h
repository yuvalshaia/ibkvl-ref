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
 * $Id: kvl.h 3443 2011-08-21 14:00:11Z saeedm $ 
 * 
 */
 
#ifndef _kvl__h_
#define _kvl__h_
#include <linux/slab.h>
#include <linux/list.h>

#define MAX_INPUT_SZ (1024)
#define OP_NAME_SZ (64)
#define OP_DESC_SZ (256)

typedef enum 
    { 
       INT,
       STR,
       BOOL
    } param_type;

struct argument {
        struct list_head list;
        const char* name;
        const char* descreption;
        char* value;
        int intdefval;
        const char* strdefval;
        param_type type;
};

typedef int (*op_execute)(void* kvlop, void *data,void* userbuff,unsigned int len);

struct kvl_op {
    struct list_head list;
    char name[OP_NAME_SZ];
    char descr[OP_DESC_SZ];
    const char* module;
    const char* user_app;
    op_execute op_run;
    void* data;
    //private data
    struct argument param_list;
};


int register_kvl_op(struct kvl_op* operation);
int unregister_kvl_op(struct kvl_op* operation);

/************************************************************************/
static inline struct kvl_op* create_kvlop(const char* name,const char* descr,const char* module,op_execute op_run,void* data,const char* user_app){
    struct kvl_op* op;
    op = kmalloc(sizeof(struct kvl_op),GFP_KERNEL);
    if (!op) {
        return NULL;
    }
    strncpy(op->name, name, OP_NAME_SZ);
    strncpy(op->descr, descr, OP_DESC_SZ);
    op->module = module;                         
    op->op_run = op_run;
    op->data   = data;               
    op->user_app = user_app;                   
    INIT_LIST_HEAD(&op->param_list.list);
    register_kvl_op(op);
    return op;
}

static inline void destroy_param_list(struct kvl_op* op){
    while(!list_empty(&op->param_list.list)) {
        struct argument* arg=(struct argument*)op->param_list.list.next;
        list_del(op->param_list.list.next);
        kfree(arg);
    }
}

static inline void destroy_kvlop(struct kvl_op* op){
    if (op){
        unregister_kvl_op(op);
        destroy_param_list(op);
        kfree(op);
    }
}

static inline int add_int_param(struct kvl_op* op,const char* name,const char* descr,int defval){
    //struct argument
    struct argument* arg;
    if (op->user_app) {
        printk("[mlxkvl] %s ERROR: cannot add argument %s when Operation have a user app [%s] configured",op->name,name,op->user_app);
        return -EINVAL;
    }
    arg = kmalloc(sizeof(struct argument),GFP_KERNEL);
    if (!arg) {
        printk("[mlxkvl] %s ERROR: Failed to allocate arguments",op->name);
        return -ENOMEM;
    }
    arg->name = name;
    arg->descreption = descr;
    arg->type = INT;
    arg->value = NULL;
    arg->intdefval = defval;
    list_add_tail(&arg->list,&op->param_list.list);
    return 0;
}

static inline int add_str_param(struct kvl_op* op,const char* name,const char* descr,const char* defval){
    struct argument* arg;
    if (op->user_app) {
        printk("[mlxkvl] %s ERROR: cannot add argument %s when Operation have a user app [%s] configured",op->name,name,op->user_app);
        return -EINVAL;
    }
    arg = kmalloc(sizeof(struct argument),GFP_KERNEL);
    if (!arg) {
        printk("[mlxkvl] ERROR: Failed to allocate arguments");
        return -ENOMEM;
    }
    arg->name = name;
    arg->descreption = descr;
    arg->type = STR;
    arg->value = NULL;
    arg->strdefval = defval;
    list_add_tail(&arg->list,&op->param_list.list);
    return 0;
}

static inline int get_param_intval(struct kvl_op* op,const char* name){
    struct list_head *cursor;
    int val= -EINVAL;
    list_for_each(cursor, &op->param_list.list) {
        struct argument* arg=(struct argument*)cursor;
        if (arg->type == INT && !strcmp(arg->name,name)){
            if (arg->value) {
                sscanf(arg->value,"%d",&val);
            }else{
                val = arg->intdefval;
            }
        }

    }
    return val;
}

static inline const char* get_param_strval(struct kvl_op* op,const char* name){
    struct list_head *cursor;
    char* val = NULL;
    list_for_each(cursor, &op->param_list.list) {
        struct argument* arg=(struct argument*)cursor;
        if (arg->type == STR && !strcmp(arg->name,name)){
            if (arg->value) {
                return arg->value;
            }else{
                return arg->strdefval;
            }
        }
    }
    return val;
 }

#endif
