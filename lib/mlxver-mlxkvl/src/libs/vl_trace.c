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
 * $Id: vl_trace.c 2302 2006-10-03 12:39:16Z dotanb $ 
 * 
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include "vl_types.h"
#include "vl_trace.h"

static unsigned long VL_verbose_flags = 0;

void VL_set_verbosity_level(
	IN		unsigned long v_level)
{
	VL_verbose_flags = v_level;
}
EXPORT_SYMBOL(VL_set_verbosity_level);

unsigned long VL_get_verbosity_level(void)
{
	return VL_verbose_flags;
}
EXPORT_SYMBOL(VL_get_verbosity_level);

/***************************************************
* Function: VL_print_test_status
***************************************************/
void VL_print_test_status(
	IN					int test_status)
{
	const char* msg;


	if (test_status == 0)
		msg = "[TEST PASSED]";
	else
		msg = "[TEST FAILED]";

	VL_PRINTF("\n\n\t-------------------------------------------\n");
	VL_PRINTF("\t\t\t%s\n", msg);
	VL_PRINTF("\t-------------------------------------------\n\n");
}
EXPORT_SYMBOL(VL_print_test_status);
