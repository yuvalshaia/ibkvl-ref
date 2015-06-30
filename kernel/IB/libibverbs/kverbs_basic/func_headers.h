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
 * $Id: func_headers.h 6191 2008-10-07 15:28:44Z vlad $ 
 * 
 */

#ifndef _FUNC_HEADERS_H_
#define _FUNC_HEADERS_H_

int test_hca(
	IN	struct config_t* config,
	IN	struct resources *res,
	IN	struct ib_client *test_client);

int test_pd(
    IN	struct config_t* config,
	IN	struct resources *res,
	IN	struct ib_client *test_client);

int test_av(
    IN	struct config_t* config,
	IN	struct resources *res,
	IN	struct ib_client *test_client);

int test_cq(
	IN	struct config_t* config,
	IN	struct resources *res,
	IN	struct ib_client *test_client);

int test_qp(
	IN	struct config_t* config,
	IN	struct resources *res,
	IN	struct ib_client *test_client);

int test_multicast(
	IN	struct config_t* config,
	IN	struct resources *res,
	IN	struct ib_client *test_client);

int test_mr(
	IN	struct config_t* config,
	IN	struct resources *res,
	IN	struct ib_client *test_client);

int test_srq(
	IN	struct config_t* config,
	IN	struct resources *res,
	IN	struct ib_client *test_client);

int test_pollpost(
	IN	struct config_t* config,
	IN	struct resources *res,
	IN	struct ib_client *test_client);


//////////////////////////////////////////////////////////////
// Helper functions
//////////////////////////////////////////////////////////////

void init_qp_cap(
	IN	struct resources *res,
	IN OUT	struct ib_qp_cap *cap);

void init_srq_cap(
	IN	struct config_t *config,
	IN	struct resources *res,
	IN OUT	struct ib_srq_init_attr *cap);

void create_random_av_attr(
	IN	struct resources *res,
	OUT	struct ib_ah_attr *attr);

void cq_comp_handler(
	IN	struct ib_cq *cq, 
	IN	void *cq_context);

void component_event_handler(
	IN	struct ib_event *event, 
	IN	void *context);

int my_modify_qp(
	IN	struct VL_random_t *rand_gen,
	IN	struct ib_qp *qp,
	IN	enum ib_qp_type type,
	IN	int port,
	IN	enum ib_qp_state state,
	IN	int connect_to_self);

int post_recv(
	IN	struct resources *res,
	IN	struct ib_qp *qp,
	IN	struct ib_mr *mr,
	IN	dma_addr_t addr,
	IN	int size);

int post_send(
	IN	struct resources *res,
	IN	struct ib_qp *qp,
	IN	struct ib_mr *mr,
	IN	dma_addr_t addr,
	IN	int size);

uint8_t get_static_rate(
	IN	int valid_value,
	IN	struct VL_random_t *rand_gen);
      
#endif
