#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(.gnu.linkonce.this_module) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section(__versions) = {
	{ 0x88389625, "module_layout" },
	{ 0x9564a2ae, "param_ops_charp" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0x7ecb001b, "__per_cpu_offset" },
	{ 0x17de3d5, "nr_cpu_ids" },
	{ 0xa1e0307f, "cpumask_next" },
	{ 0x8e3f011a, "__cpu_possible_mask" },
	{ 0x2a3b988c, "module_refcount" },
	{ 0xc5850110, "printk" },
	{ 0x9332a0e3, "find_module" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

MODULE_INFO(depends, "");

