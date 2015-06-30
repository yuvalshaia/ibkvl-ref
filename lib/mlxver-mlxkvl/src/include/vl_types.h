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
 * $Id: vl_types.h 1850 2006-07-03 11:44:26Z dotanb $
 * 
 */

#ifndef VL_TYPES_H
#define VL_TYPES_H

/*----------------------------------------------------------------*/
/* Typedefs                                                       */
/*----------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* empty macros to function headers */
#ifndef IN
#  define IN
#endif

#ifndef OUT
#  define OUT
#endif

#ifndef INOUT
#  define INOUT
#endif

#ifndef TRUE
#  define TRUE 1
#endif

#ifndef FALSE
#  define FALSE 0
#endif

/* define macros to the architecture of the CPU */
#ifdef __linux
#  if defined(__i386__)
#    define VL_ARCH_x86
#  elif defined(__x86_64__) || defined(__sparc__)
#    define VL_ARCH_x86_64
#  elif defined(__ia64__)
#    define VL_ARCH_ia64
#  elif defined(__PPC64__)
#    define VL_ARCH_ppc64
#  elif defined(__PPC__)
#    define VL_ARCH_ppc
#  else
#    error Unknown CPU architecture using the linux OS
#  endif
#elif defined(_WIN32)

    /* for now, the windows Makefile define those MACROS */
                     
#else  /* __linux */
#  error Unknown OS
#endif /* __linux */



#if defined(VL_ARCH_x86_64)
#  define U64D_FMT "%Lu"
#  define S64D_FMT "%Ld"
#  define U64H_FMT "0x%LX"
#  define VL_PID_FMT "%u"
#elif defined (VL_ARCH_ia64) || defined(VL_ARCH_ppc64)
#  define U64D_FMT "%lu"
#  define S64D_FMT "%ld"
#  define U64H_FMT "0x%lX"
#  define VL_PID_FMT "%u"
#elif defined(VL_ARCH_x86) || defined(VL_ARCH_ppc)
#  define U64D_FMT "%Lu"
#  define S64D_FMT "%Ld"
#  define U64H_FMT "0x%LX"
#  define VL_PID_FMT "%u"
#else  /* VL_ARCH */
#  error Unknown architecture
#endif /* VL_ARCH */

enum VL_ret_t {
    VL_OK = 0,
    VL_ERROR = -255,
    VL_EPERM,
    VL_EINVAL,
    VL_EAGAIN,
    VL_EINTR,
    VL_ETIMEDOUT,
    VL_EIO
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* VL_TYPES_H */

