
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
 * $Id: main.c 2841 2007-02-28 09:21:26Z yohadd $ 
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <infiniband/verbs.h>
#include <vl.h>
#include <mlxkvl/kvl_usr.h>
#include "types.h"

#define TEST_NAME "atomic_test_main"

struct VL_usage_descriptor_t usage_descriptor[] = {
	{
		'h', "help", "",
		"Print this message and exit",
		#define HELP_CMD_CASE			0
		HELP_CMD_CASE
	},

	{
		'd', "device", "HCA_ID",
		"ID of the HCA to use (Default: mlx4_0)",
		#define CMD_CASE_DEVICE			1
		CMD_CASE_DEVICE
	},

	{
		' ', "daemon", "",
		"Flag to identify the daemon host",
		#define CMD_CASE_IS_DAEMON		2
		CMD_CASE_IS_DAEMON
	},

	{
		'p', "ib_port", "PORT_NUM",
		"IB port number to use (Default: 1)",
		#define CMD_CASE_IB_PORT		3
		CMD_CASE_IB_PORT
	},

	{
		' ', "seed", "SEED",
		"Start seed for random",
		#define CMD_CASE_SEED			4
		CMD_CASE_SEED
	},

	{
		'i', "iter", "ITER",
		"Number of iterations (Default: 1000)",
		#define CMD_CASE_ITER			5
		CMD_CASE_ITER
	},

	{
		'q', "qps", "QPs",
		"Number of QPs (Default: 10)",
		#define CMD_CASE_QPS			6
		CMD_CASE_QPS
	},

	{
		'w', "wrs", "WRs",
		"Number of WRs (default: 10)",
		#define CMD_CASE_WRS			7
		CMD_CASE_WRS
	},
	
	{
		' ', "trace", "LEVEL",
		"Trace level of the test (Default: 0)",
		#define CMD_CASE_TRACE_LEVEL		8
		CMD_CASE_TRACE_LEVEL
	},

	{
		' ', "ip", "DAEMON_IP",
		"The IP of the daemon host",
		#define CMD_CASE_DAEMON_IP		9
		CMD_CASE_DAEMON_IP
	},

	{
		't', "tcp", "TCP_PORT",
		"TCP port number (Default: 19000)",
		#define CMD_CASE_TCP			10
		CMD_CASE_TCP
	},

	{
		'm', "mode", "TEST_MODE",
		"Test mode (Default: 0 (F&A), MODES: 0 (F&A), 1 (C&S), 2 (F&A/C&S), 3 (MF&A), 4 (MC&S), 5 (MF&A/MC&S))",
		#define CMD_CASE_TEST_MODE		11
		CMD_CASE_TEST_MODE
	}
};

struct config_t config = {
	"mlx4_0",	/* dev_name */
	0,		/* is_daemon */
	1,		/* ib_port */
	0,		/* seed */
	1000,		/* num_of_iter */
	10,		/* num_of_qps */
	10,		/* num_of_wrs */
	0,		/* trace_level */
	F_AND_A,	/* test_mode */
	"127.0.0.1",	/* daemon_ip */
	19000		/* tcp_port */
};

/******************************
* Function: print_config
******************************/
static void print_config(void)
{
	VL_MISC_TRACE((" %s configuration report", TEST_NAME));
	VL_MISC_TRACE((" ------------------------------------------------"));
	VL_MISC_TRACE((" Current pid                  : "VL_PID_FMT, VL_getpid()));
	VL_MISC_TRACE((" Device Name                  : \"%s\"", config.dev_name));
	VL_MISC_TRACE((" Is Daemon                    : \"%s\"", (config.is_daemon)?"YES":"NO"));
	if (!config.is_daemon)
		VL_MISC_TRACE((" Daemon IP                    : \"%s\"", config.daemon_ip));
	VL_MISC_TRACE((" IB port                      : %u", config.ib_port));
	VL_MISC_TRACE((" Random seed                  : %lu", config.seed));
	VL_MISC_TRACE((" Number of iterations         : %d", config.num_of_iter));
	VL_MISC_TRACE((" Number of QPs                : %d", config.num_of_qps));
	VL_MISC_TRACE((" Number of WRs                : %d", config.num_of_wrs));
	VL_MISC_TRACE((" Trace level                  : 0x%x", config.trace_level));
	VL_MISC_TRACE((" Test mode                    : %d", config.test_mode));
	VL_MISC_TRACE((" TCP port                     : %d", config.tcp_port));
	VL_MISC_TRACE((" ------------------------------------------------\n"));
}

/******************************
* Function: process_arg
******************************/
static int process_arg(
	IN		const int opt_index,
	IN		const char* equ_ptr,
	IN		const int arr_size,
	IN		const struct VL_usage_descriptor_t* usage_desc_arr)
{
	/* process argument */
	switch (usage_descriptor[opt_index].case_code) {
	case HELP_CMD_CASE:
		VL_usage(1, arr_size, usage_desc_arr);
		return 1;

	case CMD_CASE_DEVICE:
		config.dev_name = equ_ptr;
		break;

	case CMD_CASE_IS_DAEMON:
		config.is_daemon = 1;
		break;

	case CMD_CASE_DAEMON_IP:
		config.daemon_ip = equ_ptr;
		break;

	case CMD_CASE_IB_PORT:
		config.ib_port = strtoul(equ_ptr, NULL, 0);
		break;

	case CMD_CASE_SEED:
		config.seed = strtoul(equ_ptr, NULL, 0);
		break;

	case CMD_CASE_ITER:
		config.num_of_iter = strtoul(equ_ptr, NULL, 0);
		break;

	case CMD_CASE_QPS:
		config.num_of_qps = strtoul(equ_ptr, NULL, 0);
		break;

	case CMD_CASE_WRS:
		config.num_of_wrs = strtoul(equ_ptr, NULL, 0);
		break;

	case CMD_CASE_TRACE_LEVEL:
		config.trace_level = strtoul(equ_ptr, NULL, 0);
		break;

	case CMD_CASE_TCP:
		config.tcp_port = (unsigned short)strtoul(equ_ptr, NULL, 0);
		break;

	case CMD_CASE_TEST_MODE:
		config.test_mode = strtoul(equ_ptr, NULL, 0);
		break;

	default:
		VL_MISC_ERR(("unknown parameter is the switch '%s'\n", equ_ptr));
		exit(2);
	}

	return 0;
}

/******************************
* Function: main
******************************/
int main(int argc, char *argv[])
{
	struct u2k_cmd_t		cmd;
	struct VL_random_t		rand;
	struct VL_sock_t		socket;
	struct VL_sock_props_t		sock_props;
	struct remote_resources_t	net_remote_resources;
	struct remote_resources_t	net_local_resources;
	uint32_t 			*local_qp_num_arr = NULL, *remote_qp_num_arr = NULL;
	unsigned long			other_side_seed;
	size_t				size;
	size_t				write_rc;
	int				create_resources = 0;
	int				rc;
	int				fd = -1;
	int				test_result = 1;
	int				i;
	
	rc = VL_parse_argv(argc, argv, (sizeof(usage_descriptor)/sizeof(struct VL_usage_descriptor_t)),
			   usage_descriptor, process_arg);
	if (rc != 0)
		return rc;

	config.seed = VL_srand(config.seed, &rand);

	print_config();

	fd = kvl_dev_open(DRIVER_DEVICE_NAME);
	if (fd<=0) {
		VL_MISC_ERR(("VL_udriver_generic_open with device file '%s' failed", DRIVER_DEVICE_NAME));
		goto cleanup;
	}

	cmd.remote_resources = (struct remote_resources_t*)malloc(sizeof(struct remote_resources_t));
	if (!cmd.remote_resources){
		VL_MISC_ERR(("Failed to malloc remote_resources struct"));
		goto cleanup;
	}
	memset(cmd.remote_resources, 0, sizeof(struct remote_resources_t));

	cmd.remote_resources->qp_num_arr = (uint32_t*)malloc(sizeof(uint32_t) * config.num_of_qps);
	if (!cmd.remote_resources->qp_num_arr){
		VL_MISC_ERR(("Failed to malloc remote_resources.qp_num_arr struct"));
		goto cleanup;
	}

	/* connect between the 2 sides */
	sock_props.is_daemon = config.is_daemon;
	sock_props.port = config.tcp_port;
	strcpy(sock_props.ip, config.daemon_ip);

	VL_MISC_TRACE(("Connect to other side"));
	rc = VL_sock_connect(&sock_props, &socket);
	if (rc != VL_OK) {
		VL_SOCK_ERR(("Failed to open connection with daemon"));
		goto cleanup;
	}

	rc = VL_sock_sync_data(&socket, sizeof(unsigned long) ,
			       &config.seed, &other_side_seed);

	if (rc != VL_OK) {
		VL_SOCK_ERR(("Connection was lost when transferring seed number"));
		goto cleanup;
	}

	if (!config.is_daemon) {
		config.seed = other_side_seed;
		VL_MISC_TRACE(("Update seed to %lu", other_side_seed));
	}

	/* tell the kernel module to create resources */
	memcpy(&cmd.config, &config, sizeof(config));
	
	cmd.opcode          = CREATE_RESOURCES;
	cmd.dev_name_len    = strlen(config.dev_name) + 1;

	create_resources = 1;
	size = sizeof(cmd);
	write_rc = write(fd, &cmd, size);
	if (write_rc != size) {
		VL_MISC_ERR(("failed to create resources in kernel level"));
		goto cleanup;
	}
	VL_MISC_TRACE(("Create resources - PASS"));

	VL_MISC_TRACE(("Exchanging CM data"));

	net_local_resources.remote_addr    = VL_htonll(cmd.remote_resources->remote_addr);
	net_local_resources.rkey           = htonl(cmd.remote_resources->rkey);
	net_local_resources.lid            = htons(cmd.remote_resources->lid);
	net_local_resources.max_qp_rd_atom = htons(cmd.remote_resources->max_qp_rd_atom);

	/* exchange CM data */
	rc = VL_sock_sync_data(&socket, sizeof(struct remote_resources_t),
			       &net_local_resources, &net_remote_resources);
	if (rc != VL_OK) {
		VL_SOCK_ERR(("Connection was lost when transferring CM data"));
		goto cleanup;
	}

	cmd.remote_resources->remote_addr    = VL_ntohll(net_remote_resources.remote_addr);
	cmd.remote_resources->rkey           = ntohl(net_remote_resources.rkey);
	cmd.remote_resources->lid            = ntohs(net_remote_resources.lid);
	cmd.remote_resources->max_qp_rd_atom = ntohs(net_remote_resources.max_qp_rd_atom);

	/* exchange QPs numbers */
	VL_MISC_TRACE(("Exchanging QPs numbers"));
	size = sizeof(uint32_t) * config.num_of_qps;
	local_qp_num_arr = VL_MALLOC(size, uint32_t);
	if (!local_qp_num_arr) {
		VL_MEM_ERR(("failed to allocate %Zu bytes for local qp number array", size));
		goto cleanup;
	}

	remote_qp_num_arr = VL_MALLOC(size, uint32_t);
	if (!remote_qp_num_arr) {
		VL_MEM_ERR(("failed to allocate %Zu bytes for remote qp number array", size));
		goto cleanup;
	}

	/* fill the local_qp_num_arr array */
	for (i = 0; i < config.num_of_qps; i++)
		local_qp_num_arr[i] = htonl(cmd.remote_resources->qp_num_arr[i]);


	rc = VL_sock_sync_data(&socket, sizeof(uint32_t) * config.num_of_qps,
			       local_qp_num_arr, remote_qp_num_arr);
	if (rc != VL_OK) {
		VL_SOCK_ERR(("Connection was lost when transferring QPs numbers to daemon"));
		goto cleanup;
	}

	for (i = 0; i < config.num_of_qps; i++)
		cmd.remote_resources->qp_num_arr[i] = ntohl(remote_qp_num_arr[i]);

	/* tell the kernel module to fill qps */
	cmd.opcode          = FILL_QPS;

	size = sizeof(cmd);
	write_rc = write(fd, &cmd, size);
	if (write_rc != size) {
		VL_MISC_ERR(("failed to fill qps in kernel level"));
		goto cleanup;
	}
	VL_MISC_TRACE(("Fill QPs - PASS"));

	/* sync with other side before starting test */
	rc = VL_sock_sync_ready(&socket);
	if (rc != VL_OK) {
		VL_SOCK_ERR(("failed to sync with other side, after Fill QPs"));
		goto cleanup;
	}
	VL_SOCK_TRACE1(("sync with other side after Fill QPs"));
	
	/* tell the kernel module to run tests */
	cmd.opcode          = RUN_TEST;

	size = sizeof(cmd);
	write_rc = write(fd, &cmd, size);
	if (write_rc != size) {
		VL_MISC_ERR(("failed to run tests in kernel level"));
		goto cleanup;
	}
	VL_MISC_TRACE(("Run test - PASS"));

	/* sync with other side after test */
	rc = VL_sock_sync_ready(&socket);
	if (rc != VL_OK) {
		VL_SOCK_ERR(("failed to sync with other side, after test"));
		goto cleanup;
	}
	VL_SOCK_TRACE1(("sync with other side after test"));


	test_result = 0;

cleanup:
	if (create_resources) {
		/* tell the kernel module to destroy resources */
		cmd.opcode          = DESTROY_RESOURCES;

		size = sizeof(cmd);
		write_rc = write(fd, &cmd, size);
		if (write_rc != size) {
			VL_MISC_ERR(("failed to destroy resources in kernel level"));
			test_result = -1;
		}
	}

	rc = VL_sock_close(&socket);
	if (rc != VL_OK) {
		VL_SOCK_ERR(("failed to close connection"));
		test_result = -1;
	}


	if (cmd.remote_resources) {
		if (cmd.remote_resources->qp_num_arr)
			free(cmd.remote_resources->qp_num_arr);
		free(cmd.remote_resources);
	}

	if(remote_qp_num_arr)
		free(remote_qp_num_arr);

	if(local_qp_num_arr)
		free(local_qp_num_arr);

	if (fd != -1)
		kvl_dev_close(fd);

	VL_print_test_status(test_result);

	return test_result;
}

