#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/gpio/consumer.h>
#include <linux/timer.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/timekeeping.h>
#include <linux/interrupt.h>

#define DRIVER_NAME "hc_sr04_driver"
#define TRIGGER_PULSE_US 10
#define MEASURE_INTERVAL_MS 100
#define OUTPUT_NAME "hc_sr04_distance"

static struct gpio_desc *trigger_gpio = NULL;
static struct gpio_desc *echo_gpio = NULL;
static ktime_t start_time, end_time;
static struct timer_list measure_timer;
static struct proc_dir_entry *proc_file;
static char distance_buffer[64];
static int echo_irq;

static irqreturn_t echo_irq_handler(int irq, void *dev_id) 
{
	int gpio_state = gpiod_get_value(echo_gpio);
	printk(KERN_INFO "my_hc-sr04 - Trigger measurment %d",gpio_state);

	if (gpio_state) {
		start_time = ktime_get();
	} else {
		end_time = ktime_get();

		s64 travel_time_us = ktime_to_us(ktime_sub(end_time, start_time));

		if (travel_time_us > 0) {
			int distance_cm = travel_time_us / 58;
			snprintf(distance_buffer, sizeof(distance_buffer), "Measured distance: %d cm\n", distance_cm);
			printk(KERN_INFO "my_hc-sr04 - Calculated distance: %d cm\n", distance_cm);
        } else {
			snprintf(distance_buffer, sizeof(distance_buffer), "Error: Invalid travel time\n");
		}
	}

	return IRQ_HANDLED;
}

static void trigger_measurement(struct timer_list *t) 
{
	gpiod_set_value(trigger_gpio, 1);
	udelay(TRIGGER_PULSE_US);
	gpiod_set_value(trigger_gpio, 0);

	mod_timer(&measure_timer, jiffies + msecs_to_jiffies(MEASURE_INTERVAL_MS));
}

static ssize_t proc_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos) {
    printk(KERN_INFO "my_hc-sr04 - proc read\n");
    return simple_read_from_buffer(user_buf, count, ppos, distance_buffer, strlen(distance_buffer));
}

static const struct proc_ops proc_fops = {
	.proc_read = proc_read,
};

static int hc_sr04_probe(struct platform_device *pdev) 
{
	struct device *dev = &pdev->dev;
	int ret;

	printk(KERN_INFO "my_hc-sr04 - Probing device\n");

    trigger_gpio = gpiod_get(dev, "trigger", GPIOD_OUT_LOW);
    if (IS_ERR(trigger_gpio)) {
		dev_err(dev, "my_hc-sr04 - Failed to get trigger GPIO\n");
		return PTR_ERR(trigger_gpio);
    }

    echo_gpio = gpiod_get(dev, "echo", GPIOD_IN);
    if (IS_ERR(echo_gpio)) {
		dev_err(dev, "my_hc-sr04 - Failed to get echo GPIO\n");
		ret = PTR_ERR(echo_gpio);
		goto cleanup_trigger;
    }

    echo_irq = gpiod_to_irq(echo_gpio);
    if (echo_irq < 0) {
		dev_err(dev, "my_hc-sr04 - Failed to get IRQ for echo GPIO\n");
		ret = echo_irq;
		goto cleanup_echo;
    }

    ret = request_irq(echo_irq, echo_irq_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, DRIVER_NAME, NULL);
    if (ret) {
		dev_err(dev, "my_hc-sr04 - Failed to request IRQ\n");
		goto cleanup_echo;
    }

    proc_file = proc_create(OUTPUT_NAME, 0666, NULL, &proc_fops);
    if (!proc_file) {
		printk(KERN_ERR "my_hc-sr04 - Failed to create proc file\n");
		ret = -ENOMEM;
		goto cleanup_irq;
    }

	timer_setup(&measure_timer, trigger_measurement, 0);
	mod_timer(&measure_timer, jiffies + msecs_to_jiffies(MEASURE_INTERVAL_MS));

	printk(KERN_INFO "my_hc-sr04 - Device initialized successfully\n");
	return 0;

cleanup_irq:
	free_irq(echo_irq, NULL);
cleanup_echo:
	gpiod_put(echo_gpio);
cleanup_trigger:
	gpiod_put(trigger_gpio);
	return ret;
}

static int hc_sr04_remove(struct platform_device *pdev) 
{
	printk(KERN_INFO "my_hc-sr04 - Removing device\n");
	del_timer_sync(&measure_timer);
	remove_proc_entry(OUTPUT_NAME, NULL);
	free_irq(gpiod_to_irq(echo_gpio), NULL);
	gpiod_put(trigger_gpio);
	gpiod_put(echo_gpio);
	return 0;
}

static const struct of_device_id hc_sr04_ids[] = {
	{
		.compatible = "my_hc_sr04",
	}, { /* sentinel */ }
};

static struct platform_driver hc_sr04_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.of_match_table = hc_sr04_ids,
	},
	.probe = hc_sr04_probe,
	.remove = hc_sr04_remove,
};

static int __init my_init(void) 
{
	printk("my_hc-sr04 - Loading the driver...\n");
	return platform_driver_register(&hc_sr04_driver);
}

static void __exit my_exit(void) 
{
	printk("my_hc-sr04 - Unload driver");
	platform_driver_unregister(&hc_sr04_driver);
}

MODULE_DEVICE_TABLE(of, hc_sr04_ids);

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("-");
MODULE_DESCRIPTION("-");
