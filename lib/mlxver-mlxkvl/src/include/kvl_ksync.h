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
 * $Id: kvl_ksync.h 3443 2011-08-21 14:00:11Z saeedm $ 
 * 
 */
 
#ifndef _kvl_ksync_h_
#define _kvl_ksync_h_
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/socket.h>
#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/in.h>

#include <linux/delay.h>

//#include "kvl_common.h"

#define DEFAULT_PORT 25020
//#define CONNECT_PORT 23
//#define MODULE_NAME "ksocket"
//#define INADDR_SEND ((unsigned long int)0x7f000001) /* 127.0.0.1 */
#define INADDR_SEND INADDR_LOOPBACK
#define KSYNC_PRINT_HEADER "[mlxkvl] ksync "

#define kvl_ksync_info(fmt, args...) printk( KERN_INFO  "%s : " fmt,KSYNC_PRINT_HEADER, ## args)
#define kvl_ksync_warn(fmt, args...) printk( KERN_WARNING "%s WARNING : " fmt,KSYNC_PRINT_HEADER, ## args)
#define kvl_ksync_err(fmt, args...) printk( KERN_ERR "%s ERROR : " fmt,KSYNC_PRINT_HEADER, ## args)


typedef struct {
    struct socket* sk;
    struct socket* rsk;
    struct sockaddr_in raddr;
    bool server;
} sync_sock_t;

inline unsigned int inet_addr(char *str) 
{ 
    int a,b,c,d; 
    char arr[4]; 
    sscanf(str,"%d.%d.%d.%d",&a,&b,&c,&d); 
    arr[0] = a; arr[1] = b; arr[2] = c; arr[3] = d; 
    return *(unsigned int*)arr; 
} 

inline sync_sock_t* kvl_connect_sync(char* remote_ip,bool daemon,int retry){
    struct sockaddr_in addr;
    //struct sockaddr_in client_addr;
    struct timeval tv;
    struct socket* sync_sock=NULL;
    struct socket* remote_sock=NULL;
    int optval;
    sync_sock_t* my_socket = kmalloc(sizeof(sync_sock_t),GFP_KERNEL);
    mm_segment_t oldfs = get_fs(); 
    int err;
    int retry_count = retry;
    /* create a socket */
    if (!my_socket)
    {   
        kvl_ksync_err("Failed to allocate socket \n");    
        return NULL;
    }
    
   // int err;
    set_fs(KERNEL_DS);

    if ((err = sock_create_kern(AF_INET, SOCK_STREAM, IPPROTO_TCP, &sync_sock)) < 0)
    {
        kvl_ksync_err("Could not create a datagram socket, error = %d\n", -ENXIO);
        sync_sock = NULL;
        goto close_and_out;
    }
    kvl_ksync_info(" sync socket create success\n");
    memset(&addr, 0, sizeof(struct sockaddr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DEFAULT_PORT);
    tv.tv_sec = 10;
    tv.tv_usec = 100*1000;
    // set SO_REUSEADDR on a socket to true (1):
    optval = 1;
    if ((err=kernel_setsockopt(sync_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof optval))!=0)
    {
            kvl_ksync_err("Failed to set socket reuse option, error = %d\n", -err);
            goto close_and_out;
    }
    if ((err=kernel_setsockopt(sync_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,  sizeof tv))!=0)
    {
            kvl_ksync_err("Failed to set socket recv timeout, error = %d\n", -err);
            goto close_and_out;
    }
    if ((err=kernel_setsockopt(sync_sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv,  sizeof tv))!=0)
    {
            kvl_ksync_err("Failed to set socket send timeout, error = %d\n", -err);
            goto close_and_out;
    }

    if (daemon){
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        if ((err = kernel_bind(sync_sock, (struct sockaddr *)&addr, sizeof(struct sockaddr) ) ) < 0)
        {
            kvl_ksync_err("Could not bind socket, error = %d\n", -err);
            goto close_and_out;
        }
        
        if ((err = kernel_listen(sync_sock, 1 ) ) < 0)
        {
            kvl_ksync_err("Could not listen to socket, error = %d\n", -err);
            goto close_and_out;
        }
        kvl_ksync_info("Server : waiting for client \n");
        if ((err = kernel_accept(sync_sock,&remote_sock, sizeof(struct socket)  ) ) < 0)
        {
            kvl_ksync_err("Could not accept connection socket, error = %d\n", -err);
            remote_sock = NULL;
            goto close_and_out;
        }
        remote_sock->type = sync_sock->type;
        remote_sock->ops = sync_sock->ops;
        my_socket->sk = sync_sock;
        my_socket->rsk  = remote_sock;
        my_socket->server = daemon;
    } else {
        addr.sin_addr.s_addr = inet_addr(remote_ip);
        kvl_ksync_info("connecting to %s:%d\n", remote_ip,DEFAULT_PORT);
        while(1)
        {
            if ((err = kernel_connect(sync_sock, (struct sockaddr *)&addr, sizeof(struct sockaddr), 0)) < 0 ){
                if (retry>0){
                    retry--;
                    VL_sleep(1);
                    continue;
                }
                kvl_ksync_err("Failed to connect to %s:%d after %d sec, error = %d\n", remote_ip,DEFAULT_PORT,retry_count,-err);
                goto close_and_out;
            }
            break;
        }
        my_socket->sk = sync_sock;
        my_socket->rsk  = sync_sock;
        my_socket->raddr = addr;
        my_socket->server = daemon;
    }
    set_fs(oldfs);
    kvl_ksync_info("sync sock connected\n");
    return my_socket;
close_and_out:
    sock_release(sync_sock);
    if (remote_sock)
        sock_release(remote_sock);
    if (my_socket)
        kfree(my_socket);
    my_socket = NULL;
    sync_sock = NULL;
    remote_sock = NULL;
    set_fs(oldfs);
    return NULL;
}

inline int kvl_sock_send(sync_sock_t* sync_sock, const char *buf,const size_t length, unsigned long flags)
{
    struct msghdr msg;
    struct iovec iov;
    struct socket *sock = sync_sock->rsk;
    int len, written = 0, left = length;
    mm_segment_t oldmm;

    msg.msg_name     = 0;
    msg.msg_namelen  = 0;
    msg.msg_iov      = &iov;
    msg.msg_iovlen   = 1;
    msg.msg_control  = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags    = flags;

    oldmm = get_fs(); set_fs(KERNEL_DS);
repeat_send:
    msg.msg_iov->iov_len = left;
    msg.msg_iov->iov_base = (char *) buf +written;
    len = sock_sendmsg(sock, &msg, left);
    left = length - len;
    if (left > 0)
        goto repeat_send;
    set_fs(oldmm);
    return written ? written : len;
}

inline int kvl_sock_recv(sync_sock_t* sync_sock,char *buf,const size_t length, unsigned long flags)
{
    struct msghdr msg;
    struct iovec iov;
    struct socket *sock = sync_sock->rsk;
    int len, written = 0, left = length;
    mm_segment_t oldmm;

    msg.msg_name     = 0;
    msg.msg_namelen  = 0;
    msg.msg_iov      = &iov;
    msg.msg_iovlen   = 1;
    msg.msg_control  = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags    = flags;

    oldmm = get_fs(); set_fs(KERNEL_DS);
repeat_send:
    msg.msg_iov->iov_len = left;
    msg.msg_iov->iov_base = (char *) buf +written;
    len = sock_recvmsg(sock, &msg, left,flags);
    left = length - len;
    if (left > 0)
        goto repeat_send;
    set_fs(oldmm);
    return written ? written : len;
}


inline int kvl_ksync(sync_sock_t* sync_sock,void* in_data,void* out_data,int size){
    int rc=0;
    kvl_ksync_info("Syncing with partner ....\n");
    if (sync_sock->server) {
        rc = kvl_sock_send(sync_sock,(char*)out_data,size,0 );
        if (rc != size)
            goto err;
        rc = kvl_sock_recv(sync_sock,(char*)in_data,size,0 );
        if (rc != size)
            goto err;
    } else {
        rc = kvl_sock_recv(sync_sock,(char*)in_data,size,0 );
        if (rc != size)
            goto err;
        rc = kvl_sock_send(sync_sock,(char*)out_data,size,0);
        if (rc != size)
            goto err;
    }
    kvl_ksync_info("Syncing done\n");
    return 0;
err:
    kvl_ksync_err("Sync FAILED\n");
    return rc;
}

inline void kvl_close_sync(sync_sock_t* sync_sock){     
    if (sync_sock)
    {
        kvl_ksync_info("sync close\n");
        //sync_sock->ops->shutdown(sync_sock,2);
        //sync_sock->ops->release(sync_sock);
        //sock_close(sync_sock);
        if (sync_sock->sk){
            sync_sock->sk->ops->shutdown(sync_sock->sk,1);
            sock_release(sync_sock->sk);
        }
        if (sync_sock->server && sync_sock->rsk){
            sync_sock->rsk->ops->shutdown(sync_sock->rsk,1);
            sock_release(sync_sock->rsk);
        }
        kfree(sync_sock);
    }
    sync_sock = NULL;
}

#endif
