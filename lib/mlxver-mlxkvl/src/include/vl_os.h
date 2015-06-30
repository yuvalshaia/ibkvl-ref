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
 * $Id: vl_os.h 2477 2006-11-13 07:38:22Z dotanb $ 
 * 
 */

#ifndef VL_OS_H
#define VL_OS_H

#include "vl_types.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
# include <asm/semaphore.h>
#else
# include <linux/semaphore.h>
#endif

/*----------------------------------------------------------------*/
/* Typedefs                                                       */
/*----------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef int (*VL_thread_func_t)(void *);


struct VL_thread_t {
	struct semaphore	sync;
	int			pid;
	VL_thread_func_t	func;
	void			*func_ctx;
	int			flags;
	int			res;
};

/*----------------------------------------------------------------*/
/* Global Function Definition                                     */
/*----------------------------------------------------------------*/

/*****************************************************************************
* Function: VL_thread_start
*
* Arguments:
*    to_p(OUT)          : pointer to a structure of a thread object
*    flags(IN)          : flags for thread creation
*    tfunc(IN)          : thread main function
*    tctx(IN)           : thread context
*
* Returns:   VL_OK if thread was created, else thread creation failed
*
* Description:
*   Start a new thread.
*****************************************************************************/
enum VL_ret_t VL_thread_start(
	OUT		struct VL_thread_t *to_p,
	IN		unsigned int flags,
	IN		VL_thread_func_t tfunc,
	IN		void *tctx);

/*****************************************************************************
* Function: VL_thread_kill
*
* Arguments:
*    to_p(IN)           : pointer to a structure of a thread object
*
* Returns:   VL_OK if thread was killed, else thread kill failed
*
* Description:
*   Terminate the thread brutally
*****************************************************************************/
/* function kill_proc is no longer defined in kernel 2.6.27 or later */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
enum VL_ret_t VL_thread_kill(
	IN		struct VL_thread_t *to_p);
#endif

/*****************************************************************************
* Function: VL_thread_wait_for_term
*
* Arguments:
*    to_p(IN)           : pointer to a structure of a thread object
*    exit_code_p(OUT)   : the return value of the thread is stored in exit_code_p
*
* Returns:   VL_OK if thread was terminated, else there was an error
*
* Description:
*   Wait for a termination of a thread.
*****************************************************************************/
enum VL_ret_t VL_thread_wait_for_term(
	IN		struct VL_thread_t *to_p,
	OUT		int *exit_code_p);

/*****************************************************************************
* Function: VL_sleep
*
* Arguments:
*    sec(IN)  : time (in sec) to sleep
*
* Returns:   0 if sleep, else time left to sleep
*
* Description:
*   Sleep for the specified number of seconds.
*****************************************************************************/
unsigned int VL_sleep(
	IN		unsigned int sec);

/*****************************************************************************
* Function: VL_msleep
*
* Arguments:
*    msec(IN)  : time (in msec) to sleep
*
* Returns:   0 if sleep, else time left to sleep
*
* Description:
*   Sleep for the specified number of milliseconds.
*****************************************************************************/
unsigned int VL_msleep(
	IN		unsigned long msec);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* VL_OS_H */

