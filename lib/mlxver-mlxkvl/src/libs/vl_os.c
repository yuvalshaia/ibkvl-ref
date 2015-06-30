/*
 * Copyright (c) 2005 Mellanox Technologies. All rights reserved.
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
 * $Id: vl_os_win.c 554 2005-11-27 09:56:36Z dotanb $ 
 * 
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,5,0)
# include <linux/kthread.h>
#elif LINUX_VERSION_CODE > KERNEL_VERSION(2,6,19)
#  include <linux/sched.h>
#endif /* LINUX_VERSION_CODE*/

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
# include <asm/semaphore.h>
#else
# include <linux/semaphore.h>
#endif

#include "vl_os.h"

/***************************************************
* Function: ThreadProc
***************************************************/
static int ThreadProc(
	IN		void *args)
{
	struct VL_thread_t *to_p = (struct VL_thread_t *)args;


	/* allow sending this signal */
	allow_signal(SIGKILL);
	to_p->res = to_p->func(to_p->func_ctx);

	up(&to_p->sync);

	return to_p->res;
}

/***************************************************
* Function: VL_thread_start
***************************************************/
enum VL_ret_t VL_thread_start(
	OUT		struct VL_thread_t *to_p,
	IN		unsigned int flags,
	IN		VL_thread_func_t tfunc,
	IN		void *tctx)
{
	int res;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,5,0)
	struct task_struct *task;
#endif
	if (tfunc == NULL)
		return VL_EINVAL;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
	sema_init(&to_p->sync, 1);
#else
	init_MUTEX(&to_p->sync);
#endif
	to_p->func = tfunc;
	to_p->func_ctx = tctx;

	/* create and run the thread */

	res = down_interruptible(&to_p->sync);
	if (res) {
		return -VL_EAGAIN;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,5,0)
	task = kthread_run( ThreadProc, to_p, "VL_thread");
	to_p->pid = task->pid;
#else
	to_p->pid = kernel_thread(ThreadProc, to_p, flags);
#endif
	if (to_p->pid <= 0)
		return VL_ERROR;

	return VL_OK;
}
EXPORT_SYMBOL(VL_thread_start);

/***************************************************
* Function: VL_thread_kill
***************************************************/
/* function kill_proc is no longer defined in kernel 2.6.27 or later */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
enum VL_ret_t VL_thread_kill(
	IN		struct VL_thread_t *to_p)
{
	if (kill_proc(to_p->pid, SIGKILL, 0))
		return VL_ERROR;

	return VL_OK;
}
EXPORT_SYMBOL(VL_thread_kill);
#endif

/***************************************************
* Function: VL_thread_wait_for_term
***************************************************/
enum VL_ret_t VL_thread_wait_for_term(
	IN		struct VL_thread_t *to_p,
	OUT		int *exit_code_p)
{
	int res;
	/* when the kernel thread will finish the work, it will release the mutex */
	res = down_interruptible(&to_p->sync);
	if (res) {
		return -VL_EAGAIN;
	}
	up(&to_p->sync);

	*exit_code_p = to_p->res;

	return VL_OK;
}
EXPORT_SYMBOL(VL_thread_wait_for_term);

/***************************************************
* Function: VL_sleep
***************************************************/
unsigned int VL_sleep(
	IN		unsigned int sec)
{
	unsigned long end;


	end = jiffies + HZ * sec;

	do {
		yield();
	} while (end > jiffies);

	return 0;
}
EXPORT_SYMBOL(VL_sleep);

/***************************************************
* Function: VL_msleep
***************************************************/
unsigned int VL_msleep(
	IN		unsigned long msec)
{
	unsigned long end;


	end = jiffies + HZ * msec / 1000;

	do {
		yield();
	} while (end > jiffies);

	return 0;
}
EXPORT_SYMBOL(VL_msleep);

