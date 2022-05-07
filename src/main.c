#include <linux/init.h> /* Needed for the macros */
#include <linux/kernel.h> /* Needed for pr_info, snprintf */
#include <linux/module.h> /* Needed by all modules */
#include <linux/fs.h> /* Needed for alloc_chrdev_region */
#include <linux/cdev.h> /* Needed by chardev */
#include <linux/device.h>
#include <linux/smp.h> /* Needed for on_each_cpu */
#include <linux/cpumask.h>/* Needed for on_each_cpu */
#include <linux/types.h> /* Needed for uint64_t, etc */
#include <linux/string.h> /* Needed for strncpy, etc */
#include <linux/uaccess.h> /* Needed for copy_from_user, copy_to_user */

#include "cpu.h"
#include "vm.h"

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Totsugekitai");
MODULE_DESCRIPTION("Totsugekitai Hypervisor from scratch");

#define SUCCESS 0
#define DEVICE_NAME "tvisor"
#define KBUF_SIZE 512

enum {
	CDEV_NOT_USED = 0,
	CDEV_EXCLUSIVE_OPEN = 1,
};

struct tvisor_state {
	int is_virtualization_ready;
	int is_vmx_enabled;
};

static int major;
static struct class *cls;
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);
struct tvisor_state TVISOR_STATE = {
	.is_virtualization_ready = 0,
	.is_vmx_enabled = 0,
};

vm_state_t *VM = NULL;

static int tvisor_open(struct inode *, struct file *);
static int tvisor_release(struct inode *, struct file *);
static ssize_t tvisor_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t tvisor_write(struct file *, const char __user *, size_t,
			    loff_t *);

static struct file_operations tvisor_fops = {
	.open = tvisor_open,
	.release = tvisor_release,
	.read = tvisor_read,
	.write = tvisor_write,
};

static int tvisor_open(struct inode *inode, struct file *file)
{
	if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN)) {
		return -EBUSY;
	}

	pr_info("tvisor: open\n");
	try_module_get(THIS_MODULE);

	return SUCCESS;
}

static int tvisor_release(struct inode *inode, struct file *file)
{
	atomic_set(&already_open, CDEV_NOT_USED);

	module_put(THIS_MODULE);

	pr_info("tvisor: release\n");

	return SUCCESS;
}

static ssize_t tvisor_read(struct file *filp, char __user *ubuf, size_t count,
			   loff_t *offset)
{
	char kbuf[KBUF_SIZE] = { '\0' };
	size_t len;
	int nchar;

	nchar = snprintf(
		kbuf, KBUF_SIZE,
		"tvisor: virtualization ready: %d\nVMX is enabled: %d\n",
		TVISOR_STATE.is_virtualization_ready,
		TVISOR_STATE.is_vmx_enabled);
	if (nchar >= KBUF_SIZE) {
		pr_alert("tvisor: snprintf truncated!!!\n");
	}

	len = strlen(kbuf);
	if ((count - *offset) + len <= count) {
		return 0;
	}

	if (copy_to_user(ubuf, kbuf, len)) {
		return -EFAULT;
	}

	pr_info("tvisor: read[%s]\n", kbuf);

	*offset += len;

	return len;
}

static ssize_t tvisor_write(struct file *filp, const char __user *ubuf,
			    size_t count, loff_t *offset)
{
	const char *enable = "enable";
	const char *disable = "disable";
	const char *create = "create";
	const char *destroy = "destroy";
	const char *launch = "launch";

	char kbuf[KBUF_SIZE] = { '\0' };

	if (copy_from_user(kbuf, ubuf, count)) {
		return -EFAULT;
	}
	pr_info("tvisor: write[%s]\n", kbuf);

	if (!strncmp(kbuf, enable, strlen(enable))) {
		if (VM == NULL) {
			pr_info("tvisor: please create VM\n");
		} else {
			int err = enable_vmx_on_each_cpu_mask(0,
							      VM->vmxon_region);
			if (err) {
				pr_alert("tvisor: failed to enable VMX[%d]\n",
					 err);
			} else {
				TVISOR_STATE.is_vmx_enabled = 1;
				pr_info("tvisor: enable VMX!\n");
			}
		}
	} else if (!strncmp(kbuf, disable, strlen(disable))) {
		if (TVISOR_STATE.is_vmx_enabled) {
			int err = disable_vmx_on_each_cpu_mask(0);
			if (err) {
				pr_alert("tvisor: failed to disable VMX\n");
			} else {
				TVISOR_STATE.is_vmx_enabled = 0;
				pr_info("tvisor: disable VMX!\n");
			}
		} else {
			pr_info("tvisor: vmx is not enabled now\n");
		}
	} else if (!strncmp(kbuf, create, strlen(create))) {
		VM = create_vm();
		if (VM == NULL) {
			pr_alert("tvisor: failed to create_vm\n");
		} else {
			pr_info("tvisor: create VM\n");
		}
	} else if (!strncmp(kbuf, destroy, strlen(destroy))) {
		destroy_vm(VM);
		pr_info("tvisor: destroy VM\n");
	} else if (!strncmp(kbuf, launch, strlen(launch))) {
		if (TVISOR_STATE.is_vmx_enabled) {
			VM = create_vm();
			if (VM == NULL) {
				pr_alert("tvisor: failed to create_vm\n");
			} else {
				launch_vm(0, VM); // Launch VM on CPU 0
			}
		} else {
			pr_info("tvisor: VMX is not enabled\n");
		}
	}

	return count;
}

static int __init init_tvisor(void)
{
	pr_info("tvisor: hello!\n");

	major = register_chrdev(0, DEVICE_NAME, &tvisor_fops);
	if (major < 0) {
		pr_alert("Registering character device failed[%d]\n", major);
		return major;
	}

	pr_debug("tvisor: assigned major number[%d]\n", major);

	cls = class_create(THIS_MODULE, DEVICE_NAME);
	device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);

	pr_debug("tvisor: Device created on /dev/%s\n", DEVICE_NAME);

	return SUCCESS;
}

static void __exit exit_tvisor(void)
{
	if (TVISOR_STATE.is_vmx_enabled) {
		int err = disable_vmx_on_each_cpu_mask(0);
		if (err) {
			pr_alert("tvisor: failed to disable VMX\n");
		} else {
			pr_info("tvisor: disable VMX!\n");
		}
	}
	// free_vmxon_region(vm.vmxon_region);

	device_destroy(cls, MKDEV(major, 0));
	class_destroy(cls);

	unregister_chrdev(major, DEVICE_NAME);

	pr_info("tvisor: bye!\n");
}

module_init(init_tvisor);
module_exit(exit_tvisor);
