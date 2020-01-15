#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/interrupt.h>

#define DEVICE_NAME "gemmini"
#define GEMMINI_EXEC 1
#define IRQ_NO 2

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Max Banister");

dev_t dev;
struct class *device_class;
struct cdev my_cdev;

struct mutex gemmini_hart_lock;
wait_queue_head_t gemmini_wait_queue;
static int done;

int my_open(struct inode *, struct file *);
long my_ioctl(struct file *, unsigned int, unsigned long);
/*static int my_release(struct inode *, struct file *);*/
irqreturn_t gemmini_interrupt_handler(int, void *);

const struct file_operations fops = {
	.owner          = THIS_MODULE,
	.open           = my_open,
	.unlocked_ioctl = my_ioctl,
	/*.release        = my_release*/
};

int __init my_init(void) {
	int ret;
	struct device *device;

	ret = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
	if (ret < 0) {
		pr_warn("Error: alloc_chrdev_region\n");
	}
	pr_info("Major No: %d, Minor No: %d\n", MAJOR(dev), MINOR(dev));

	device_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(device_class)) {
		pr_warn("Class creation failed\n");
		unregister_chrdev_region(dev, 1);
		return PTR_ERR(device_class);
	}

	device = device_create(device_class, NULL, dev, NULL, DEVICE_NAME);
	if (IS_ERR(device)) {
		pr_warn("Device creation failed\n");
		class_destroy(device_class);
		unregister_chrdev_region(dev, 1);
		return PTR_ERR(device);
	}

	cdev_init(&my_cdev, &fops);
	my_cdev.owner = THIS_MODULE;

	/* All initialization must be completed by this point */
	if ((ret = cdev_add(&my_cdev, dev, 1)) < 0) {
		pr_warn("cdev_add failed: %d\n", ret);
		device_destroy(device_class, dev);
		class_destroy(device_class);
		unregister_chrdev_region(dev, 1);
		return ret;
	}
	mutex_init(&gemmini_hart_lock);
	init_waitqueue_head(&gemmini_wait_queue);
	
	ret = request_irq(IRQ_NO, gemmini_interrupt_handler, IRQF_SHARED, DEVICE_NAME, gemmini_interrupt_handler);
	if (ret < 0) {
		pr_warn("Could not request irq no %d: %d\n", IRQ_NO, ret);
		cdev_del(&my_cdev);
		device_destroy(device_class, dev);
		class_destroy(device_class);
		unregister_chrdev_region(dev, 1);
		return ret;
	}

	pr_info("Test module successfully loaded.");
	return 0;
}

void __exit my_exit(void) {
	cdev_del(&my_cdev);
	device_destroy(device_class, dev);
	class_destroy(device_class);
	unregister_chrdev_region(dev, 1);
	free_irq(IRQ_NO, gemmini_interrupt_handler);
	pr_info("Unloading test module.\n");
}

int my_open(struct inode *inp, struct file *filp) {
	pr_info("Inside the %s function", __FUNCTION__);
	return 0;
}

long my_ioctl(struct file *fp, unsigned int cmd, unsigned long arg) {
	pr_info("Function pointer %p received\n", (void (*)(void))arg);
	switch (cmd) {
	case GEMMINI_EXEC:
		pr_info("Acquiring lock...\n");
		if ((mutex_lock_interruptible(&gemmini_hart_lock)) < 0) {
			/* User interrupted the process */
			return -EINTR;
		}
		pr_info("Lock acquired!\n");
		/* Pretend we interrupt other hart here */

		/* Only one process should be waiting at a time */
		done = 0;
		if ((wait_event_interruptible_timeout(gemmini_wait_queue, done != 0, 1000)) < 0) {
			mutex_unlock(&gemmini_hart_lock);
			return -EINTR;
		}
		pr_info("Finished processing\n");
		mutex_unlock(&gemmini_hart_lock);
		break;
	default:
		return -ENOTTY;
	}
	return cmd;
}

/*int my_release(struct inode *inp, struct file *filp) {
    printk(KERN_ALERT "Inside the %s function", __FUNCTION__);
    return 0;
}*/

irqreturn_t gemmini_interrupt_handler(int irq_no, void *dev_id) {
	pr_info("Interrupt received - waking process\n");
	done = 1;
	wake_up_interruptible(&gemmini_wait_queue);
	/* clear interrupt-pending bit */
	return IRQ_HANDLED;
}

module_init(my_init);
module_exit(my_exit);
