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
 * $Id: vl_utils.h 4908 2008-01-23 06:58:47Z dotanb $
 *
 */

#ifndef VL_UTILS_H
#define VL_UTILS_H

/* macros to handle masks */
#define VL_MASK_CLR_ALL(mask)           ((mask)=0)
#define VL_MASK_SET_ALL(mask)           ((mask)=(0x07FFFFFF))
#define VL_MASK_SET(mask, attr)         ((mask)|=(attr))
#define VL_MASK_CLEAR(mask, attr)       ((mask)&=(~(attr)))
#define VL_MASK_IS_SET(mask, attr)      (((mask)&(attr))!=0)
#define VL_MASK_IS_CLEAR(mask, attr)    (((mask)&(attr))==0)

/* macros to handle min, max */
#define VL_MAX(val1, val2) (((val1) > (val2)) ? (val1) : (val2))
#define VL_MIN(val1, val2) (((val1) < (val2)) ? (val1) : (val2))

/* macro to print value of enumerations */
#define VL_CASE_SETSTR(e)  case e: s = #e; break;

/* macro to handle address align */
#define VL_ALIGN_DOWN(value, mask)      ((value) & (~((typeof(value))0) << (typeof(value))(mask)))
#define VL_ALIGN_UP(value, mask)        VL_ALIGN_DOWN((((typeof(value))1 << (typeof(value))(mask)) - 1) + (value), mask)


#endif /* VL_UTILS_H */

