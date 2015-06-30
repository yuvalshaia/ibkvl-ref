#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xb17ed07c, "module_layout" },
	{ 0x5a34a45c, "__kmalloc" },
	{ 0xd6ee688f, "vmalloc" },
	{ 0xed7a75d4, "VL_ib_wr_opcode_str" },
	{ 0x758f5ca4, "ib_dealloc_pd" },
	{ 0xf15faef6, "malloc_sizes" },
	{ 0x5fb1d894, "ib_destroy_qp" },
	{ 0x5d6aaa8f, "VL_get_verbosity_level" },
	{ 0x999e8297, "vfree" },
	{ 0x7d11c268, "jiffies" },
	{ 0x343a1a8, "__list_add" },
	{ 0xbfc38927, "ib_modify_qp" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x59c42dcd, "ib_create_qp" },
	{ 0x1e491a04, "ib_unmap_fmr" },
	{ 0x3695280e, "ib_alloc_pd" },
	{ 0xde0bdcff, "memset" },
	{ 0xd3574f6c, "ib_query_device" },
	{ 0x985558a1, "printk" },
	{ 0x42224298, "sscanf" },
	{ 0x7ec9bfbc, "strncpy" },
	{ 0xb4390f9a, "mcount" },
	{ 0x1347cd6d, "ib_query_port" },
	{ 0xa08e8821, "unregister_kvl_op" },
	{ 0xf0d3e4e8, "VL_thread_wait_for_term" },
	{ 0x521445b, "list_del" },
	{ 0x1a925a66, "down" },
	{ 0xaf572227, "ib_destroy_cq" },
	{ 0x7d545da, "VL_thread_start" },
	{ 0x225e19b9, "ib_register_client" },
	{ 0x95aa3622, "ib_dealloc_fmr" },
	{ 0xbdeae9b4, "VL_print_test_status" },
	{ 0x1000e51, "schedule" },
	{ 0x91e26184, "VL_random" },
	{ 0xe63818eb, "VL_set_verbosity_level" },
	{ 0x5c6a3e35, "VL_srand" },
	{ 0x7f24de73, "jiffies_to_usecs" },
	{ 0x68539f7c, "ib_alloc_fmr" },
	{ 0xebf1d59e, "kmem_cache_alloc_trace" },
	{ 0xb4a72604, "register_kvl_op" },
	{ 0xe52947e7, "__phys_addr" },
	{ 0x1a4f24c0, "VL_range" },
	{ 0x37a0cba, "kfree" },
	{ 0x57b09822, "up" },
	{ 0x9eef09f0, "VL_ib_wc_status_str" },
	{ 0xa8a89c5b, "ib_create_cq" },
	{ 0x397ed4f, "ib_unregister_client" },
	{ 0x584afee4, "dma_ops" },
	{ 0x760a0f4f, "yield" },
	{ 0x16bb8589, "VL_ib_port_state_str" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=mlxkvl,ib_core";


MODULE_INFO(srcversion, "1E4FA063D865758C6DBCE05");
