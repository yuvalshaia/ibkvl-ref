#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include "pfmr_test.h"

#define DRIVER_AUTHOR "Saeed Mahameed <saeedm@mellanox.com> " \
		      "Yuval Shaia <yuval.shaia@oracle.com>";
#define DRIVER_DESC   "Protected FMRs test"

int mlnx_command = 0;
module_param_named(command, mlnx_command, int, 0644);
MODULE_PARM_DESC(command, "Enable debug tracing if > 0");

static void pfmr_add_one(struct ib_device *device);
static void pfmr_remove_one(struct ib_device *device);


struct ib_client test_client = {
	.name   = "pfmr_ktest",
	.add    = pfmr_add_one,
	.remove = pfmr_remove_one
};

static void pfmr_add_one(struct ib_device *device)
{
	char opname[64];
	struct pfmr_test_ctx *test_ctx = pfmr_create_test_ctx(device);
	debug("adding IB device %s ...\n", device->name);
	if (!test_ctx) {
		error("Failed to create test context");
		return;
	}
	ib_set_client_data(device, &test_client, test_ctx);

	sprintf(opname, "%s-create_fmr", device->name);
	test_ctx->create_fmr_op = create_kvlop(opname, "create FMRs",
					       MODULE_NAME, pfmr_create_fmr,
					       test_ctx, NULL);
	if (!test_ctx->create_fmr_op) {
		error("Failed to create FMRs OP for :%s\n",device->name);
		return;
	}
	add_int_param(test_ctx->create_fmr_op, "nfmr", "num FMRs", 1);
	add_int_param(test_ctx->create_fmr_op, "max_pages", "Maximum pages for FMR", 10);
	add_int_param(test_ctx->create_fmr_op, "max_maps", "Maximum remaps for FMR", 10);
	add_int_param(test_ctx->create_fmr_op, "show", "show fmrs", 0);

	sprintf(opname, "%s-destroy_fmr", device->name);
	test_ctx->destroy_fmr_op = create_kvlop(opname, "destroy FMRs",
						MODULE_NAME, pfmr_destroy_fmr,
						test_ctx, NULL);
	if (!test_ctx->destroy_fmr_op) {
		error("Failed to create FMR destroy OP for %s\n", device->name);
		return;
	}
	add_int_param(test_ctx->destroy_fmr_op, "fmr_idx", "index of FMR", 0);

	sprintf(opname, "%s-create_dma", device->name);
	test_ctx->create_dma_op = create_kvlop(opname, "create DMA buffers",
					       MODULE_NAME,
					       pfmr_create_dma_buff, test_ctx,
					       NULL);
	if (!test_ctx->create_dma_op) {
		error("Failed to create DMA OP for :%s\n", device->name);
		return;
	}
	add_int_param(test_ctx->create_dma_op, "ndma", "num DMA buffers", 1);
	add_int_param(test_ctx->create_dma_op, "show", "show dma buffers", 0);

	sprintf(opname, "%s-dma", device->name);
	test_ctx->destroy_dma_op = create_kvlop(opname, "DMA buffer operations",
						MODULE_NAME, pfmr_dma_ops,
						test_ctx, NULL);
	if (!test_ctx->destroy_dma_op) {
		error("Failed to create DMA OP for :%s\n",device->name);
		return;
	}
	add_int_param(test_ctx->destroy_dma_op, "dma_idx", "DMA index", 0);
	add_int_param(test_ctx->destroy_dma_op, "des", "Destroy DMA", 0);
	add_int_param(test_ctx->destroy_dma_op, "get", "Read value", 0);
	add_str_param(test_ctx->destroy_dma_op, "set", "Set value", "~");

	sprintf(opname, "%s-fmr_map", device->name);
	test_ctx->fmr_map_op = create_kvlop(opname, "MAP FMRs", MODULE_NAME,
					    pfmr_map_dma_fmr, test_ctx, NULL);
	if (!test_ctx->fmr_map_op) {
		error("Failed to create MAP FMRs OP for :%s\n",device->name);
		return;
	}
	add_int_param(test_ctx->fmr_map_op , "fmr_idx", "index of FMR", 0);
	add_int_param(test_ctx->fmr_map_op , "dma_idx", "DMA index of dma "
		      "address", 0);
	add_int_param(test_ctx->fmr_map_op, "unmap", "unmap", 0);
	add_int_param(test_ctx->fmr_map_op, "show", "show mappings", 0);

	sprintf(opname, "%s-fmr_pool", device->name);
	test_ctx->fmr_pool_op = create_kvlop(opname, "Create FMR Pool",
					     MODULE_NAME, pfmr_create_pool,
					     test_ctx, NULL);
	if (!test_ctx->fmr_pool_op) {
		error("Failed to create FMR POOL OP for :%s\n", device->name);
		return;
	}
	add_int_param(test_ctx->fmr_pool_op, "size", "size of FMR pool", 1024);

	sprintf(opname, "%s-cq", device->name);
	test_ctx->cq_op = create_kvlop(opname, "CQ",
				       MODULE_NAME, pfmr_cq_op,
				       test_ctx, NULL);
	if (!test_ctx->cq_op) {
		error("Failed to create CQ OP for :%s\n", device->name);
		return;
	}
	add_int_param(test_ctx->cq_op, "show", "Show CQ list", 0);
	add_int_param(test_ctx->cq_op, "cr", "Create CQ", 0);
	add_int_param(test_ctx->cq_op, "des", "Destroty CQ", 0);

	sprintf(opname, "%s-qp", device->name);
	test_ctx->qp_op = create_kvlop(opname, "QP",
				       MODULE_NAME, pfmr_qp_op,
				       test_ctx, NULL);
	if (!test_ctx->qp_op) {
		error("Failed to create QP OP for :%s\n", device->name);
		return;
	}
	add_int_param(test_ctx->qp_op, "show", "Show QP list", 0);
	add_int_param(test_ctx->qp_op, "des", "Destroty QP", 0);
	add_int_param(test_ctx->qp_op, "cr", "Create QP(scq_idx, rcq_idx)", 0);
	add_int_param(test_ctx->qp_op, "scq_idx", "Index of send CQ", 0);
	add_int_param(test_ctx->qp_op, "rcq_idx", "Index of recv CQ", 0);
	add_int_param(test_ctx->qp_op, "rts", "Change QP state RTS (port, dlid,"
				       " dqpn)", 0);
	add_int_param(test_ctx->qp_op, "port", "Source port", 0);
	add_int_param(test_ctx->qp_op, "dlid", "Dest LID", 0);
	add_int_param(test_ctx->qp_op, "dqpn_idx", "Destination QP", 0);
	add_str_param(test_ctx->qp_op, "dqpn", "Destination QP number", 0);
	add_int_param(test_ctx->qp_op, "rdma", "Send memory to dest (fmr_idx, "
				       "rfmr_idx)", 0);
	add_int_param(test_ctx->qp_op , "fmr_idx", "Index of FMR", 0);
	add_int_param(test_ctx->qp_op , "rfmr_idx", "Remote FMR index", 0);
	add_str_param(test_ctx->qp_op , "rkey", "Remote key", 0);
	add_str_param(test_ctx->qp_op , "raddr", "Remote address", 0);

	sprintf(opname, "%s-complete_test", device->name);
	test_ctx->complete_test_op = create_kvlop(opname,
						  "Complete FMR test",
						  MODULE_NAME,
						  pfmr_complete_test,
						  test_ctx, NULL);
	if (!test_ctx->complete_test_op) {
		error("Failed to create Complete FMR OP for %s\n",
		      device->name);
		return;
	}

	info("pfmr test ops created for IB device %s...\n", device->name);
}

static void pfmr_remove_one(struct ib_device *device)
{
	struct pfmr_test_ctx *test_ctx = ib_get_client_data(device,
							    &test_client);
	if (test_ctx) {
		if (test_ctx->fmr_map_op) {
			info("MAP FMR OP: %s was removed for IB device %s...\n",
			     test_ctx->fmr_map_op->name, device->name);
			destroy_kvlop(test_ctx->fmr_map_op);
		}
		if (test_ctx->create_fmr_op) {
			info("Create FMR OP: %s was removed for IB device %s.."
			     ".\n", test_ctx->create_fmr_op->name,
			     device->name);
			destroy_kvlop(test_ctx->create_fmr_op);
		}
		if (test_ctx->destroy_fmr_op) {
			info("Create FMR OP: %s was removed for IB device %s.."
			     ".\n", test_ctx->destroy_fmr_op->name,
			     device->name);
			destroy_kvlop(test_ctx->destroy_fmr_op);
		}
		if (test_ctx->create_dma_op) {
			info("Create DMA OP : %s was removed for IB device %s.."
			     ".\n", test_ctx->create_dma_op->name,
			     device->name);
			destroy_kvlop(test_ctx->create_dma_op);
		}
		if (test_ctx->destroy_dma_op) {
			info("Create FMR OP: %s was removed for IB device %s.."
			     ".\n", test_ctx->destroy_dma_op->name,
			     device->name);
			destroy_kvlop(test_ctx->destroy_dma_op);
		}
		if (test_ctx->fmr_pool_op) {
			info("Create DMA OP : %s was removed for IB device %s.."
			     ".\n", test_ctx->fmr_pool_op->name, device->name);
			destroy_kvlop(test_ctx->fmr_pool_op);
		}
		if (test_ctx->cq_op) {
			info("CQ OP : %s was removed for IB device %s.."
			     ".\n", test_ctx->cq_op->name, device->name);
			destroy_kvlop(test_ctx->cq_op);
		}
		if (test_ctx->qp_op) {
			info("QP OP : %s was removed for IB device %s.."
			     ".\n", test_ctx->qp_op->name, device->name);
			destroy_kvlop(test_ctx->qp_op);
		}
		if (test_ctx->complete_test_op) {
			info("QP OP : %s was removed for IB device %s.."
			     ".\n", test_ctx->complete_test_op->name,
			     device->name);
			destroy_kvlop(test_ctx->complete_test_op);
		}
	}
	if (test_ctx)
		pfmr_destroy_test_ctx(test_ctx);
	info("IB device %s removed successfuly\n", device->name);
}


static int init_pfmr(void)
{
	int rc;
	rc = ib_register_client(&test_client);
	if (rc) {
		error("failed register IB client rc=%d\n",rc);
		return -ENODEV;
	}
	info("Protected FMRs test is Loaded\n");
	return 0;
}


static void cleanup_pfmr(void)
{
	ib_unregister_client(&test_client);
	info("Protected FMRs test is Unloaded\n");
}


module_init(init_pfmr);
module_exit(cleanup_pfmr);

/* Get rid of taint message by declaring code as GPL. */
MODULE_LICENSE("GPL");

MODULE_AUTHOR(DRIVER_AUTHOR);    /* Who wrote this module? */
MODULE_DESCRIPTION(DRIVER_DESC); /* What does this module do? */

/*
This module uses /dev/testdevice. The MODULE_SUPPORTED_DEVICE macro might be
used in the future to help automatic configuration of modules, but is currently
unused other than for documentation purposes.
*/
MODULE_SUPPORTED_DEVICE("testdevice");

