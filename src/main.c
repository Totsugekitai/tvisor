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
#include "vmx.h"

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
static struct tvisor_state state = {
	.is_virtualization_ready = 0,
	.is_vmx_enabled = 0,
};

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
		state.is_virtualization_ready, state.is_vmx_enabled);
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

	char kbuf[KBUF_SIZE] = { '\0' };

	if (copy_from_user(kbuf, ubuf, count)) {
		return -EFAULT;
	}
	pr_info("tvisor: write[%s]\n", kbuf);

	if (!strncmp(kbuf, enable, strlen(enable))) {
		pr_info("tvisor: enable VMX!\n");
		run_vmx();
		state.is_vmx_enabled = 1;
	} else if (!strncmp(kbuf, disable, strlen(disable))) {
		pr_info("tvisor: disable VMX!\n");
		exit_vmx();
		state.is_vmx_enabled = 0;
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

	pr_info("tvisor: assigned major number[%d]\n", major);

	cls = class_create(THIS_MODULE, DEVICE_NAME);
	device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);

	pr_info("tvisor: Device created on /dev/%s\n", DEVICE_NAME);

	int err = init_vmx();
	if (err) {
		pr_alert("tvisor: init_vmx failed[%d]\n", err);
		return err;
	}

	return SUCCESS;
}

static void __exit exit_tvisor(void)
{
	if (state.is_vmx_enabled) {
		exit_vmx();
	}

	device_destroy(cls, MKDEV(major, 0));
	class_destroy(cls);

	unregister_chrdev(major, DEVICE_NAME);

	pr_info("tvisor: bye!\n");
}

module_init(init_tvisor);
module_exit(exit_tvisor);
