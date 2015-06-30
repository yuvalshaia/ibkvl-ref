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
 * $Id: vl_rand.c 2467 2006-11-08 10:04:33Z dotanb $ 
 * 
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,19)
# include <linux/jiffies.h>
#endif /* LINUX_VERSION_CODE */
#include <linux/fs.h>       /* for register char device */
#include <asm/div64.h>
#include "vl_rand.h"


static int
VL__srand48_r (long int seedval, struct VL_random_t *buffer)
{
  /* The standards say we only have 32 bits.  */
  if (sizeof (long int) > 4)
    seedval &= 0xffffffffl;

  buffer->__x[2] = seedval >> 16;
  buffer->__x[1] = seedval & 0xffffl;
  buffer->__x[0] = 0x330e;

  buffer->__a = 0x5deece66dull;
  buffer->__c = 0xb;
  buffer->__init_arr = 1;

  return 0;
}

static int
VL__drand48_iterate (unsigned short int xsubi[3], struct VL_random_t *buffer)
{
  u64 X;
  u64 result;

  /* Initialize buffer, if not yet done.  */
  if (__builtin_expect (!buffer->__init_arr, 0))
    {
      buffer->__a = 0x5deece66dull;
      buffer->__c = 0xb;
      buffer->__init_arr = 1;
    }

  /* Do the real work.  We choose a data type which contains at least
     48 bits.  Because we compute the modulus it does not care how
     many bits really are computed.  */

  X = (u64) xsubi[2] << 32 | (u32) xsubi[1] << 16 | xsubi[0];

  result = X * buffer->__a + buffer->__c;

  xsubi[0] = result & 0xffff;
  xsubi[1] = (result >> 16) & 0xffff;
  xsubi[2] = (result >> 32) & 0xffff;

  return 0;
}

static int
VL__nrand48_r (unsigned short int xsubi[3], struct VL_random_t *buffer, long int *result)
{
  /* Compute next state.  */
  if (VL__drand48_iterate (xsubi, buffer) < 0)
    return -1;

  /* Store the result.  */
  if (sizeof (unsigned short int) == 2)
    *result = xsubi[2] << 15 | xsubi[1] >> 1;
  else
    *result = xsubi[2] >> 1;

  return 0;
}


/***************************************************
* Function: VL_srand
***************************************************/
unsigned long VL_srand(
    IN                  unsigned long seed,
    INOUT               struct VL_random_t* rand_p)
{
    unsigned long used_seed;


    if (!seed)
        used_seed = jiffies;
    else
        used_seed = seed;

    VL__srand48_r(used_seed, rand_p);

    return VL_get_seed(rand_p);
}
EXPORT_SYMBOL(VL_srand);

/***************************************************
* Function: VL_get_seed
***************************************************/
unsigned long VL_get_seed(
    IN                  const struct VL_random_t* rand_p)
{
    unsigned long used_seed = 0;

    used_seed = rand_p->__x[1] & 0xffff;
    used_seed |= (rand_p->__x[2] & 0xffff) << 16;

    return used_seed;
}
EXPORT_SYMBOL(VL_get_seed);

/***************************************************
* Function: VL_random
***************************************************/
unsigned int VL_random(
    INOUT               struct VL_random_t* rand_p,
    IN                  unsigned long end)
{
    long int rand_value1;
    long int rand_value2;

    /* make sure that we won't devide a number with zero */
    if (!end)
        return 0;

    (void) VL__nrand48_r (rand_p->__x, rand_p, &rand_value1);
    (void) VL__nrand48_r (rand_p->__x, rand_p, &rand_value2);

    rand_value1 |= (rand_value2 & 0x1) << 31;

    return rand_value1 % end;
}
EXPORT_SYMBOL(VL_random);

/***************************************************
* Function: VL_random64
***************************************************/
/*
in the url: http://www.captain.at/howto-udivdi3-umoddi3.php you can find the following text:

__udivdi3 __umoddi3 - 64 bit division in linux

If you've encountered an error message like this

Unknown symbol __udivdi3
Unknown symbol __umoddi3
Unresolved symbol __udivdi3
Unresolved symbol __umoddi3

you most likely want to make a 64 bit division, which is not supported by default in linux kernel space.

To solve this problem, you need to use the do_div macro available in asm/div64.h:

#include <asm/div64.h>
unsigned long long x, y, result;
unsigned long mod;
mod = do_div(x, y);
result = x;

If you want to calculate x / y with do_div(x, y), the result of the division is in x, the remainder is returned from the do_
div function.
*/
u64 VL_random64(
    INOUT               struct VL_random_t* rand_p,
    IN                  u_int64_t end)
{
    u64 tmp1;
    long int rand_value1;
    long int rand_value2;
    long int rand_value3;

    /* make sure that we won't devide a number with zero */
    if (!end)
        return 0;

    (void) VL__nrand48_r (rand_p->__x, rand_p, &rand_value1);
    (void) VL__nrand48_r (rand_p->__x, rand_p, &rand_value2);
    (void) VL__nrand48_r (rand_p->__x, rand_p, &rand_value3);

    rand_value1 |= (rand_value3 & 0x1) << 31;
    rand_value2 |= (rand_value3 & 0x2) << 30;

    tmp1 = ((((u64)rand_value1) << 32) | (u64)rand_value2);

    return do_div(tmp1, end);
}
EXPORT_SYMBOL(VL_random64);

/***************************************************
* Function: VL_range
***************************************************/
unsigned int VL_range(
    INOUT               struct VL_random_t* rand_p,
    IN                  unsigned long start,
    IN                  unsigned long end)
{
    /* if the values are equal, return one of them */
    if (start == end)
        return start;

    return (VL_random(rand_p, end - start + 1) + start);
}
EXPORT_SYMBOL(VL_range);

/***************************************************
* Function: VL_range64
***************************************************/
u64 VL_range64(
    INOUT               struct VL_random_t* rand_p,
    IN                  u64 start,
    IN                  u64 end)
{
    /* if the values are equal, return one of them */
    if (start == end)
        return start;

    return (VL_random64(rand_p, end - start + 1) + start);
}
EXPORT_SYMBOL(VL_range64);
