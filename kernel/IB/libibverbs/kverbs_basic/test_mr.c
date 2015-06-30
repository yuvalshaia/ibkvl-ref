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
 * $Id: test_mr.c 7877 2009-07-29 14:49:45Z ronniz $ 
 * 
 */
 
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h>       /* for register char device */
#include <asm/page.h>
#include <linux/mm.h>

#include <vl_ib_verbs.h>

#include <vl.h>
#include "my_types.h"
#include "func_headers.h"

// TODO:
// 1. query MR
// 2. rereg phys
// 3. dreg with MW

int my_kmalloc(
	IN	int size,
	IN	int align_to_page,
	OUT	void** buf,
	OUT	void** use_buffer)
{
	int alloc_size = size;

	if (align_to_page)
		alloc_size += PAGE_SIZE - (size % PAGE_SIZE);

	*buf = kmalloc(alloc_size, GFP_DMA);
	CHECK_PTR("kmalloc", *buf, return -1);

	if (align_to_page)
		*use_buffer = (void*)(unsigned long)PAGE_ALIGN((u64)(unsigned long)(*buf));
	else
		*use_buffer = *buf;

	return 0;
}

int my_query_mr(
	IN	struct ib_mr *mr,
	IN	struct ib_pd *pd,
	IN	void *buf,
	IN	int size,
	IN	int acl)
{
	struct ib_mr_attr 	attr;
	int			rc;

	rc = ib_query_mr(mr, &attr);
	CHECK_VALUE("ib_query_mr", rc, 0, return -1);

	CHECK_VALUE("device_virt_addr", attr.device_virt_addr, (unsigned long)&mr->device, return -1);
	CHECK_VALUE("lkey", attr.lkey, mr->lkey, return -1);
	CHECK_VALUE("rkey", attr.rkey, mr->rkey, return -1);
	CHECK_VALUE("mr_access_flags", attr.mr_access_flags, acl, return -1);
	CHECK_VALUE("size", attr.size, size, return -1);
	CHECK_VALUE("pd", (unsigned long)attr.pd, (unsigned long)pd, return -1);

	return 0;
}

/* ib_get_dma_mr ib_dereg_mr */
int mr_1(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_pd 		*pd = ERR_PTR(-EINVAL);
	struct ib_mr		*mr = ERR_PTR(-EINVAL);
	dma_addr_t		dma = 0;
	void			*alloc_buf = NULL;
	void			*use_buf = NULL;
	int			size = 0;
	int			i;
	int			j;
	int			acl;
	int			maped = 0;
	int			result = -1;
	int			rc;

	TEST_CASE("ib_get_dma_mr ib_dereg_mr");

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	size = VL_range(&res->rand_g, 1, PAGE_SIZE * 2);

	rc = my_kmalloc(size, 0, &alloc_buf, &use_buf);
	CHECK_VALUE("my_kmalloc", rc, 0, goto cleanup);

	for (i = DMA_BIDIRECTIONAL; i <= DMA_FROM_DEVICE; i++) {
		dma = ib_dma_map_single(res->device, use_buf, size, i);
		CHECK_VALUE("ib_dma_map_single", ib_dma_mapping_error(res->device, dma), 0, goto cleanup);
		maped = 1 + i;

		for (j = 0; j < 3; ++j) {
			switch(j) {
			case 0:
				acl = IB_ACCESS_LOCAL_WRITE;
				if (VL_random(&res->rand_g, 2))
				    acl |= IB_ACCESS_REMOTE_WRITE;
				if (VL_random(&res->rand_g, 2))
				    acl |= IB_ACCESS_REMOTE_ATOMIC;
				break;
			case 1:
				acl = IB_ACCESS_REMOTE_READ;
				break;
			case 2:
				acl = IB_ACCESS_MW_BIND;
				break;
			default:
				goto cleanup;
			}

			mr = ib_get_dma_mr(pd, acl);
			CHECK_PTR("ib_get_dma_mr", !IS_ERR(mr), goto cleanup);

		    /*  NOT IMPLIMENTED YET  
			rc = my_query_mr(mr, pd, use_buf, size, acl);
			CHECK_VALUE("my_query_mr", rc, 0, goto cleanup);*/

			rc = ib_dereg_mr(mr);
			CHECK_VALUE("ib_dereg_mr", rc, 0, return -1);
			mr = ERR_PTR(-EINVAL);
		}

		ib_dma_unmap_single(res->device, dma, size, i);
		maped = 0;
        }

	PASSED;

	result = 0;

cleanup:


	if (!IS_ERR(pd)) {
		rc = ib_dealloc_pd(pd);
		CHECK_VALUE("ib_dealloc_pd", rc, 0, return -1);
	}

	if (alloc_buf) {
		if (maped)
			ib_dma_unmap_single(res->device, dma, size, maped - 1);
		kfree(alloc_buf);
	}

	return result;
}

/* ib_alloc_fmr ib_map_phys_fmr ib_unmap_fmr ib_dealloc_fmr */
int mr_2(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_pd 		*pd = ERR_PTR(-EINVAL);
	struct ib_fmr		*fmr = ERR_PTR(-EINVAL);
	struct ib_fmr_attr	fmr_attr;
	void			*alloc_buf = NULL;
	void			*use_buf = NULL;
	struct scatterlist	*sg = NULL;
	dma_addr_t		*dma_array = NULL;
	int			size;
	int			num_pages;
	int			sg_num_pages = 0;
	int			i;
	int			j;
	int			k;
	int			loop;
	int			acl;
	int			maped = 0;
	int			result = -1;
	int			rc;

	TEST_CASE("ib_alloc_fmr ib_map_phys_fmr ib_unmap_fmr ib_dealloc_fmr");

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	size = VL_range(&res->rand_g, 1, PAGE_SIZE * 7);
	num_pages = size / PAGE_SIZE;
	if (num_pages == 0)
		num_pages = 1;

	fmr_attr.max_pages = num_pages * 2;
	fmr_attr.max_maps = VL_range(&res->rand_g, 1, res->device_attr.max_map_per_fmr);
	fmr_attr.page_shift = PAGE_SHIFT;

	sg_num_pages = VL_range(&res->rand_g, num_pages, num_pages * 2);

	sg = kzalloc(sg_num_pages * sizeof(struct scatterlist), GFP_KERNEL);
	CHECK_PTR("malloc", sg, goto cleanup);

	dma_array = kmalloc(sg_num_pages * sizeof(dma_addr_t), GFP_KERNEL);
	CHECK_PTR("malloc", dma_array, goto cleanup);

	rc = my_kmalloc(size, 1, &alloc_buf, &use_buf);
	CHECK_VALUE("my_kmalloc", rc, 0, goto cleanup);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
	sg_init_table(sg, sg_num_pages);
#endif

	for (i = 0; i < sg_num_pages; i++) {
		struct page *page;
		unsigned int len, offset;
		int buf_offset = VL_range(&res->rand_g, 1, size);

		page = virt_to_page(use_buf + buf_offset);
		len = (size - buf_offset > PAGE_SIZE) ? 
			VL_range(&res->rand_g, 1, PAGE_SIZE) : VL_range(&res->rand_g, 1, size - buf_offset);
		offset = (size - buf_offset > PAGE_SIZE) ?
			VL_range(&res->rand_g, 0, PAGE_SIZE - sg[i].length) : VL_range(&res->rand_g, 0, size - buf_offset - sg[i].length);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
		sg[i].page   = page;
		sg[i].length = len;
		sg[i].offset = offset;
#else
		sg_set_page(&sg[i], page, len, offset);
#endif
	}

	for (i = DMA_BIDIRECTIONAL; i <= DMA_FROM_DEVICE; i++) {
		rc = ib_dma_map_sg(res->device, sg, sg_num_pages, i);
		CHECK_VALUE("ib_dma_map_sg", rc, sg_num_pages, goto cleanup);
		maped = 1 + i;

		for (j = 0; j < sg_num_pages; ++j)
			dma_array[j] = sg[j].dma_address;

		for (j = 0; j < 3; ++j) {
			switch(j) {
			case 0:
				acl = IB_ACCESS_LOCAL_WRITE;
				if (VL_random(&res->rand_g, 2))
				    acl |= IB_ACCESS_REMOTE_WRITE;
				if (VL_random(&res->rand_g, 2))
				    acl |= IB_ACCESS_REMOTE_ATOMIC;
				break;
			case 1:
				acl = IB_ACCESS_REMOTE_READ;
				break;
			case 2:
				acl = IB_ACCESS_MW_BIND;
				break;
			default:
				goto cleanup;
			}
			
			fmr = ib_alloc_fmr(pd, acl, &fmr_attr);
			CHECK_PTR("ib_alloc_fmr", !IS_ERR(fmr), goto cleanup);

			loop = VL_range(&res->rand_g, 1, 10);

			for (k = 0; k < loop; ++k) {
				struct list_head fmr_list;

				rc = ib_map_phys_fmr(fmr, dma_array, sg_num_pages, (u64) (unsigned long)use_buf);
				CHECK_VALUE("ib_map_phys_fmr", rc, 0, goto cleanup);
	
				INIT_LIST_HEAD (&fmr_list);
				list_add (&fmr->list, &fmr_list);
	
				rc = ib_unmap_fmr(&fmr_list);
				CHECK_VALUE("ib_unmap_fmr", rc, 0, goto cleanup);
			}

                        rc = ib_dealloc_fmr(fmr);
			CHECK_VALUE("ib_dealloc_fmr", rc, 0, goto cleanup);
		}
				
		ib_dma_unmap_sg(res->device, sg, sg_num_pages, i);
		maped = 0;
        }

	PASSED;

	result = 0;

cleanup:


	if (!IS_ERR(pd)) {
		rc = ib_dealloc_pd(pd);
		CHECK_VALUE("ib_dealloc_pd", rc, 0, return -1);
	}

	if (sg) {
		if (maped)
			ib_dma_unmap_sg(res->device, sg, sg_num_pages, maped - 1);
		kfree(sg);
	}

	if (dma_array)
		kfree(dma_array);

	if (alloc_buf)
		kfree(alloc_buf);

	return result;
}

/* ib_reg_phys_mr */
int mr_3(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	struct ib_pd 		*pd = ERR_PTR(-EINVAL);
	struct ib_mr		*mr = ERR_PTR(-EINVAL);
	dma_addr_t		dma = 0;
	void			*alloc_buf = NULL;
	void			*use_buf = NULL;
	struct ib_phys_buf	*buf_array = NULL;
	u64 			iova;
	int			size = 0;
	int			num_pages;
	int			i;
	int			j;
	int			acl;
	int			maped = 0;
	int			result = -1;
	int			rc;

	TEST_CASE("ib_reg_phys_mr");

	pd = ib_alloc_pd(res->device);
	CHECK_PTR("ib_alloc_pd", !IS_ERR(pd), goto cleanup);

	size = VL_range(&res->rand_g, 1, PAGE_SIZE * 7);
	num_pages = size / PAGE_SIZE;
	if (num_pages == 0)
		num_pages = 1;

	buf_array = kmalloc(num_pages * sizeof(struct ib_phys_buf), GFP_KERNEL);
	CHECK_PTR("malloc", buf_array, goto cleanup);
        
	rc = my_kmalloc(size, 1, &alloc_buf, &use_buf);
	CHECK_VALUE("my_kmalloc", rc, 0, goto cleanup);

	for (i = DMA_BIDIRECTIONAL; i <= DMA_FROM_DEVICE; i++) {
		dma = ib_dma_map_single(res->device, use_buf, size, i);
		CHECK_VALUE("ib_dma_map_single", ib_dma_mapping_error(res->device, dma), 0, goto cleanup);
		maped = 1 + i;

		for (j = 0; j < num_pages; ++j) {
			buf_array[j].addr = dma + PAGE_SIZE * j;
			buf_array[j].size = PAGE_SIZE;
		}
		buf_array[num_pages - 1].size = size - PAGE_SIZE * (num_pages - 1);

		for (j = 0; j < 3; ++j) {
			switch(j) {
			case 0:
				acl = IB_ACCESS_LOCAL_WRITE;
				if (VL_random(&res->rand_g, 2))
				    acl |= IB_ACCESS_REMOTE_WRITE;
				if (VL_random(&res->rand_g, 2))
				    acl |= IB_ACCESS_REMOTE_ATOMIC;
				break;
			case 1:
				acl = IB_ACCESS_REMOTE_READ;
				break;
			case 2:
				acl = IB_ACCESS_MW_BIND;
				break;
			default:
				goto cleanup;
			}

			iova = buf_array[0].addr;

			mr = ib_reg_phys_mr(pd, buf_array, num_pages, acl, &iova);
			CHECK_PTR("ib_reg_phys_mr", !IS_ERR(mr), VL_MEM_ERR(("error %ld",PTR_ERR(mr))); goto cleanup);

			rc = ib_dereg_mr(mr);
			CHECK_VALUE("ib_dereg_mr", rc, 0, goto cleanup);
			mr = ERR_PTR(-EINVAL);
		}
				
		ib_dma_unmap_single(res->device, dma, size, i);
		maped = 0;
        }

	PASSED;

	result = 0;

cleanup:


	if (!IS_ERR(pd)) {
		rc = ib_dealloc_pd(pd);
		CHECK_VALUE("ib_dealloc_pd", rc, 0, return -1);
	}

	if (buf_array)
		kfree(buf_array);

	if (alloc_buf) {
		if (maped)
			ib_dma_unmap_single(res->device, dma, size, maped - 1);
		kfree(alloc_buf);
	}

	return result;
}


int test_mr(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN	struct ib_client *test_client)
{
	RUN_TEST(1, mr_1);
	RUN_TEST(2, mr_2);
	RUN_TEST(3, mr_3);

	return 0;
}
