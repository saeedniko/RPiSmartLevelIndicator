#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/ioctl.h>

#define SET_TIMER_INTERVAL _IOW('a', 'a', int32_t*)

static unsigned long TIMEOUT = 3000;
#define DEVICE_NAME "my_timer_device"

static struct timer_list my_timer;
static int dev_open(struct inode *inode, struct file *file);
static int dev_release(struct inode *inode, struct file *file);
void timer_callback(struct timer_list *data);
static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static struct cdev cdev;
static int dev_major = 0;
static struct class *dev_class = NULL;

static int dev_uevent(const struct device *dev, struct kobj_uevent_env *env)
{
    add_uevent_var(env, "DEVMODE=%#o", 0666);
    return 0;
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_release,
    .unlocked_ioctl = dev_ioctl,
};



static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
    case SET_TIMER_INTERVAL: {
        int32_t interval;
        if (copy_from_user(&interval, (int32_t*)arg, sizeof(int32_t)))
            return -EFAULT;

        if (interval < 0 || interval > INT_MAX)
            return -EINVAL;

        TIMEOUT = interval;
        mod_timer(&my_timer, jiffies + msecs_to_jiffies(TIMEOUT));
        printk(KERN_INFO "Timer interval set to %d ms\n", TIMEOUT);
        break;
    }
    default:
        return -ENOTTY;
    }
    return 0;
}
static int __init dev_init(void)
{
    int err;
    dev_t dev;

    err = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
    if (err < 0) {
        printk(KERN_ERR "Failed to allocate character device region\n");
        return err;
    }

    dev_major = MAJOR(dev);

    dev_class = class_create(DEVICE_NAME);
    if (IS_ERR(dev_class)) {
        printk(KERN_ERR "Failed to create device class\n");
        err = PTR_ERR(dev_class);
        goto unregister_chrdev;
    }

    dev_class->dev_uevent = dev_uevent;

    cdev_init(&cdev, &fops);
    cdev.owner = THIS_MODULE;

    err = cdev_add(&cdev, MKDEV(dev_major, 0), 1);
    if (err < 0) {
        printk(KERN_ERR "Failed to add character device\n");
        goto destroy_class;
    }

    struct device *dev_obj;

    dev_obj = device_create(dev_class, NULL, MKDEV(dev_major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(dev_obj)) {
        printk(KERN_ERR "Failed to create device\n");
        err = PTR_ERR(dev_obj);
        goto remove_cdev;
    }

    printk(KERN_INFO "Hello from %s\n", __func__);

    timer_setup(&my_timer, timer_callback, 0);
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(TIMEOUT));

    return 0;

remove_cdev:
    cdev_del(&cdev);
destroy_class:
    class_destroy(dev_class);
unregister_chrdev:
    unregister_chrdev_region(MKDEV(dev_major, 0), MINORMASK);

    return err;
}
static void __exit dev_exit(void)
{
    del_timer(&my_timer);
    device_destroy(dev_class, MKDEV(dev_major, 0));
    class_unregister(dev_class);
    class_destroy(dev_class);
    unregister_chrdev_region(MKDEV(dev_major, 0), MINORMASK);
    printk(KERN_INFO "Good Bye from %s\n", __func__);
}

static int dev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Device open\n");
    return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Device close\n");
    return 0;
}

void timer_callback(struct timer_list *data)
{
    pr_info("Timer Callback! %s\n", __func__);

    mod_timer(&my_timer, jiffies + msecs_to_jiffies(TIMEOUT));
}

module_init(dev_init);
module_exit(dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("GPIO Driver");
MODULE_VERSION("1.0");
