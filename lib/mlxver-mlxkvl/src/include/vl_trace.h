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
 * $Id: vl_trace.h 2302 2006-10-03 12:39:16Z dotanb $ 
 *
 */

#ifndef VL_TRACE_H
#define VL_TRACE_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include "vl_types.h"

#define VL_PRINTF printk

#define VL_PRINT(color, pre, prin) { \
      VL_PRINTF("%20s , %3d %10s :", strrchr(__FILE__, '/') ,__LINE__ ,pre);    \
      VL_PRINTF prin;                                             \
      VL_PRINTF("\n");                                            \
      }

#define VL_PRINT_COND(flg, color, pre, prin)					\
      do {									\
            if (((VL_get_verbosity_level() & flg) != 0) ||( flg == 0UL))	\
                   VL_PRINT(color, pre, prin)					\
      } while (0)

/* Error prints */
#define VL_DATA_ERR(prin)       VL_PRINT_COND(0      , COLOR_RED, " DATA_ERR      " ,prin)
#define VL_MEM_ERR(prin)        VL_PRINT_COND(0      , COLOR_RED, " MEM_ERR       " ,prin)
#define VL_HCA_ERR(prin)        VL_PRINT_COND(0      , COLOR_RED, " HCA_ERR       " ,prin)
#define VL_MISC_ERR(prin)       VL_PRINT_COND(0      , COLOR_RED, " MISC_ERR      " ,prin)
#define VL_SOCK_ERR(prin)       VL_PRINT_COND(0      , COLOR_RED, " SOCK_ERR      " ,prin)

/* Trace prints (printed always) */
#define VL_DATA_TRACE(prin)     VL_PRINT_COND(0      , COLOR_BLUE,  " DATA_TRACE   " ,prin)
#define VL_MEM_TRACE(prin)      VL_PRINT_COND(0      , COLOR_BLUE,  " MEM_TRACE    " ,prin)
#define VL_HCA_TRACE(prin)      VL_PRINT_COND(0      , COLOR_BLUE,  " HCA_TRACE    " ,prin)
#define VL_MISC_TRACE(prin)     VL_PRINT_COND(0      , COLOR_BLUE,  " MISC_TRACE   " ,prin)
#define VL_SOCK_TRACE(prin)     VL_PRINT_COND(0      , COLOR_BLUE,  " SOCK_TRACE   " ,prin)

/* Trace prints (depend on trace level) */
#define VL_DATA_TRACE1(prin)    VL_PRINT_COND((1<<0) , COLOR_GREEN, " DATA_TRACE1  " ,prin)
#define VL_MEM_TRACE1(prin)     VL_PRINT_COND((1<<1) , COLOR_GREEN, " MEM_TRACE1   " ,prin)
#define VL_HCA_TRACE1(prin)     VL_PRINT_COND((1<<2) , COLOR_GREEN, " HCA_TRACE1   " ,prin)
#define VL_MISC_TRACE1(prin)    VL_PRINT_COND((1<<3) , COLOR_GREEN, " MISC_TRACE1  " ,prin)
#define VL_SOCK_TRACE1(prin)    VL_PRINT_COND((1<<4) , COLOR_GREEN, " SOCK_TRACE1  " ,prin)


void VL_set_verbosity_level(
        IN              unsigned long v_level);

unsigned long VL_get_verbosity_level(void);

/*****************************************************************************
* Function: VL_print_test_status
*
* Arguments:
*     test_status(IN)  : status of the test: 0 test passed, else test failed.
*
* Returns: None
*
* Description:
*   This function print the test status.
*****************************************************************************/
void VL_print_test_status(
    IN                  int test_status);

#endif /* VL_TRACE_H */

