#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/interrupt.h>

#define DEVICE_NAME "gemmini"
#define GEMMINI_EXEC 1
#define IRQ_NO 2

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Max Banister");

struct mutex gemmini_hart_lock;
wait_queue_head_t gemmini_wait_queue;
static int done;

int gem_open(struct inode *, struct file *);
long gem_ioctl(struct file *, unsigned int, unsigned long);
/*static int gem_release(struct inode *, struct file *);*/
irqreturn_t gemmini_interrupt_handler(int, void *);

const struct file_operations fops = {
	.owner          = THIS_MODULE,
	.open           = gem_open,
	.unlocked_ioctl = gem_ioctl,
	/*.release        = gem_release*/
};

struct miscdevice gemmini_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &fops,
};

int __init gem_init(void) {
	int ret;

	ret = misc_register(&gemmini_device);
	if (ret < 0) {
		pr_err("Unable to register %s device\n, err=%d", DEVICE_NAME, ret);
		return ret;
	}

	mutex_init(&gemmini_hart_lock);
	init_waitqueue_head(&gemmini_wait_queue);
	
	ret = request_irq(IRQ_NO, gemmini_interrupt_handler, IRQF_SHARED, DEVICE_NAME, gemmini_interrupt_handler);
	if (ret < 0) {
		pr_err("Could not request irq no %d: err=%d\n", IRQ_NO, ret);
		return ret;
	}

	pr_info("Test module successfully loaded.");
	return 0;
}

void __exit gem_exit(void) {
	misc_deregister(&gemmini_device);
	free_irq(IRQ_NO, gemmini_interrupt_handler);
	pr_info("Unloading test module.\n");
}

int gem_open(struct inode *inp, struct file *filp) {
	pr_info("Inside the %s function", __FUNCTION__);
	return 0;
}

long gem_ioctl(struct file *fp, unsigned int cmd, unsigned long arg) {
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

/*int gem_release(struct inode *inp, struct file *filp) {
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

module_init(gem_init);
module_exit(gem_exit);
