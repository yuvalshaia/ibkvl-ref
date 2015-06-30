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
 * $Id: vl_rand.h 2106 2006-08-31 10:40:48Z dotanb $ 
 * 
 */

#ifndef VL_RAND_H
#define VL_RAND_H

#include <linux/module.h>
#include <linux/kernel.h>
#include "vl_types.h"

/*----------------------------------------------------------------*/
/* Typedefs                                                       */
/*----------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* the random implementation is taken from glibc-2.5 source code */
struct VL_random_t {
    unsigned short int __x[3];  /* Current state.  */
    unsigned short int __old_x[3]; /* Old state.  */
    unsigned short int __c;     /* Additive const. in congruential formula.  */
    unsigned short int __init_arr;/* Flag for initializing.  */
    unsigned long long int __a; /* Factor in congruential formula.  */
};


/*----------------------------------------------------------------*/
/* Global Function Definition                                     */
/*----------------------------------------------------------------*/

/*****************************************************************************
* Function: VL_srand
*
* Arguments:
*     seed(IN)  : seed number to use: if 0-put a random seed, else use the is seed
*     rand_p(IN/OUT) : pointer to the random data structure 
*
* Returns: the seed number
*
* Description:
*   This function set a seed value to the random engine.
*****************************************************************************/
unsigned long VL_srand(
    IN                  unsigned long seed,
    OUT                 struct VL_random_t* rand_p);

/*****************************************************************************
* Function: VL_get_seed
*
* Arguments:
*     rand_p(IN) : pointer to the random data structure  
*
* Returns: the seed number
*
* Description:
*   This function return the current random seed from the random struct..
*****************************************************************************/
unsigned long VL_get_seed(
    IN                  const struct VL_random_t* rand_p);

/*****************************************************************************
* Function: VL_random
*
* Arguments:
*    rand_p(IN/OUT) : pointer to the random data structure 
*    end(IN)  : maximum-1 value to use
*
* Returns:   a random number [0..end-1]
*
* Description:
*   Return a random integer value between 0 and end-1.
*****************************************************************************/
unsigned int VL_random(
    INOUT               struct VL_random_t* rand_p,
    IN                  unsigned long end);

/*****************************************************************************
* Function: VL_random64
*
* Arguments:
*    rand_p(IN/OUT) : pointer to the random data structure
*    end(IN)  : maximum-1 value to use
*
* Returns:   a random number [0..end-1]
*
* Description:
*   Return a random 64 bit value between 0 and end-1.
*****************************************************************************/
u64 VL_random64(
    INOUT               struct VL_random_t* rand_p,
    IN                  u64 end);

/*****************************************************************************
* Function: VL_range
*
* Arguments:
*    rand_p(IN/OUT) : pointer to the random data structure 
*    start(IN): minimum value of the range
*    end(IN)  : maximum value of the range
*
* Returns:   a random number [start..end]
*
* Description:
*   Return a random integer value between start and end.
*****************************************************************************/
unsigned int VL_range(
    INOUT               struct VL_random_t* rand_p,
    IN                  unsigned long start,
    IN                  unsigned long end);

/*****************************************************************************
* Function: VL_range64
*
* Arguments:
*    rand_p(IN/OUT) : pointer to the random data structure
*    start(IN): minimum value of the range
*    end(IN)  : maximum value of the range
*
* Returns:   a random number [start..end]
*
* Description:
*   Return a random 64 bit value between start and end.
*****************************************************************************/
u64 VL_range64(
    INOUT               struct VL_random_t* rand_p,
    IN                  u64 start,
    IN                  u64 end);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* VL_RAND_H */

