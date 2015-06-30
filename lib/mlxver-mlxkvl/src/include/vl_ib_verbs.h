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
 * $Id: vl_gen2k_str.h 114 2005-09-19 14:00:17Z dotanb $ 
 * 
 */

#ifndef VL_IB_VERBS_H
#define VL_IB_VERBS_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,14)
# if LINUX_VERSION_CODE != KERNEL_VERSION(2,6,9)
   typedef unsigned gfp_t;
# endif
# if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
#  include <asm/io.h>
#  include <asm/scatterlist.h>
# endif
/*# include <linux/device.h>*/
/*# include <linux/mm.h>*/
#endif /* LINUX_VERSION_CODE */

#if defined(__ia64__)
#include <linux/pci.h>
#endif

#include <ib_verbs.h>

#endif /* VL_IB_VERBS_H */

