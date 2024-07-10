#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define TRIG_PIN 23
#define ECHO_PIN 24

static dev_t dev;
static struct cdev ultrasonic_cdev;
static struct class *ultrasonic_class;
static struct device *ultrasonic_device;

static long distance;

static int ultrasonic_open(struct inode *inode, struct file *file)
{
    // Initialize GPIO pins
    gpio_request(TRIG_PIN, "ultrasonic_trig");
    gpio_request(ECHO_PIN, "ultrasonic_echo");
    gpio_direction_output(TRIG_PIN, 0);
    gpio_direction_input(ECHO_PIN);

    return 0;
}

static int ultrasonic_release(struct inode *inode, struct file *file)
{
    // Clean up GPIO pins
    gpio_free(TRIG_PIN);
    gpio_free(ECHO_PIN);

    return 0;
}

static ssize_t ultrasonic_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    int ret;
    char distance_str[20];

    sprintf(distance_str, "%ld\n", distance);

    ret = copy_to_user(buf, distance_str, strlen(distance_str));
    if (ret)
        return -EFAULT;

    return strlen(distance_str);
}

static ssize_t ultrasonic_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    // Not implemented for this example
    return -ENOTSUPP;
}

static struct file_operations ultrasonic_fops = {
    .owner = THIS_MODULE,
    .open = ultrasonic_open,
    .release = ultrasonic_release,
    .read = ultrasonic_read,
    .write = ultrasonic_write,
};

static int __init ultrasonic_init(void)
{
    int ret;

    printk(KERN_INFO "Ultrasonic driver: initializing\n");

    // Register character device
    ret = alloc_chrdev_region(&dev, 0, 1, "ultrasonic");
    if (ret < 0) {
        printk(KERN_ERR "Failed to allocate character device region\n");
        return ret;
    }

    cdev_init(&ultrasonic_cdev, &ultrasonic_fops);
    ret = cdev_add(&ultrasonic_cdev, dev, 1);
    if (ret < 0) {
        unregister_chrdev_region(dev, 1);
        return ret;
    }

    ultrasonic_class = class_create(THIS_MODULE, "ultrasonic");
    if (IS_ERR(ultrasonic_class)) {
        cdev_del(&ultrasonic_cdev);
        unregister_chrdev_region(dev, 1);
        return PTR_ERR(ultrasonic_class);
    }

    ultrasonic_device = device_create(ultrasonic_class, NULL, dev, NULL, "ultrasonic");
    if (IS_ERR(ultrasonic_device)) {
        class_destroy(ultrasonic_class);
        cdev_del(&ultrasonic_cdev);
        unregister_chrdev_region(dev, 1);
        return PTR_ERR(ultrasonic_device);
    }

    return 0;
}

static void __exit ultrasonic_exit(void)
{
    device_destroy(ultrasonic_class, dev);
    class_destroy(ultrasonic_class);
    cdev_del(&ultrasonic_cdev);
    unregister_chrdev_region(dev, 1);

    printk(KERN_INFO "Ultrasonic driver: exiting\n");
}

module_init(ultrasonic_init);
module_exit(ultrasonic_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Ultrasonic driver for Raspberry Pi");
