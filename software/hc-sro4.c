/*
 * SOLUTION: Uebung 4: Implementation einer GPIO Schnittstelle
 * In dieser Übung soll mittels Button-Interrupts eine LED an einem separaten Pin an- und ausgeschaltet werden.
 * Hierzu ist keine Userapp notwendig, da die ganze Logik auf im Kernelspace abläuft.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/mod_devicetable.h>
#include <linux/property.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/gpio/consumer.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>

/*
 * Platform driver function declarations
 */
static int dt_probe(struct platform_device *pdev);
static int dt_remove(struct platform_device *pdev);
static ssize_t my_write(struct file *File, const char *user_buffer, size_t count, loff_t *offs);

/*
 * Platform driver global variables
 */
static struct proc_dir_entry *proc_file;

/*
 * GPIO related global variables
 */
static struct gpio_desc *my_led = NULL;
static struct gpio_desc *my_interrupt = NULL;
static irqreturn_t my_interrupt_handler(int, void *dev_id);
const char* gpio_string = "my_pins";

/*
 * Implementation of interrupt handler
 */
static irqreturn_t my_interrupt_handler(int irq, void *dev_id)
{
    printk("dt_gpio - Interrupt received!\n");
    
    // Toggle the LED GPIO
    if (!IS_ERR_OR_NULL(my_led)) {
        int value = gpiod_get_value(my_led);
        gpiod_set_value(my_led, !value);
    } else {
        printk("dt_gpio - Error: LED GPIO not initialized\n");
    }

    return IRQ_HANDLED;
}

/*
 * Platform driver operatations
 */
static struct proc_ops fops = {
	.proc_write = my_write,
};

/*
 * Define an array of device IDs that are compatible.
 * This will be matched in the device tree in the
 * probe() function later.
 */
static struct of_device_id my_driver_ids[] = {
	{
		.compatible = "brightlight,mydev",
	}, { /* sentinel */ }
};

/*
 * Define platform driver instance with desired
 * functions (probe and remove in this case)
 */
static struct platform_driver my_driver = {
	.probe = dt_probe,
	.remove = dt_remove,
	.driver = {
		.name = "my_device_driver",
		.of_match_table = my_driver_ids,
	},
};

/*
 * Write function for the platform driver.
 * This function is called when data is written to the associated file.
 */
static ssize_t my_write(struct file *File, const char *user_buffer, size_t count, loff_t *offs) {
	switch (user_buffer[0]) {
		case '0':
		case '1':
			gpiod_set_value(my_led, user_buffer[0] - '0');
		default:
			break;
	}
	return count;
}

/*
 * Probe function for the platform device.
 * This function is called when the device is beeing probed.
 */
static int dt_probe(struct platform_device *pdev) {
	struct device *dev = &pdev->dev;
	const char *label;
	int my_value, ret;

	printk("dt_gpio - Now I am in the probe function!\n");

	/* Check for device properties */
	if(!device_property_present(dev, "label")) {
		printk("dt_gpio - Error! Device property 'label' not found!\n");
		return -1;
	}
	if(!device_property_present(dev, "my_value")) {
		printk("dt_gpio - Error! Device property 'my_value' not found!\n");
		return -1;
	}
	if(!device_property_present(dev, "my_pins-gpio")) {
		printk("dt_gpio - Error! Device property 'my_pins-gpio' not found!\n");
		return -1;
	}

	/* Read device properties */
	ret = device_property_read_string(dev, "label", &label);
	if(ret) {
		printk("dt_gpio - Error! Could not read 'label'\n");
		return -1;
	}
	printk("dt_gpio - label: %s\n", label);
	ret = device_property_read_u32(dev, "my_value", &my_value);
	if(ret) {
		printk("dt_gpio - Error! Could not read 'my_value'\n");
		return -1;
	}
	printk("dt_gpio - my_value: %d\n", my_value);

	/* Init GPIO */
	my_led = gpiod_get_index(dev, gpio_string, 0, GPIOD_OUT_LOW);
	if(IS_ERR(my_led)) {
		printk("dt_gpio - Error! Could not setup the GPIO\n");
		return -1 * IS_ERR(my_led);
	}
	printk("Initialized everything!\n");

    my_interrupt = gpiod_get_index(dev, gpio_string, 1, GPIOD_IN);
        if (IS_ERR(my_interrupt)) {
            printk("dt_gpio - Error! Could not get the interrupt GPIO\n");
            ret = PTR_ERR(my_interrupt);
            goto cleanup;
        }

	int irq = gpiod_to_irq(my_interrupt);

	
	ret = request_irq(irq, my_interrupt_handler, IRQF_TRIGGER_FALLING, "my_device", NULL);
        if (ret) {
            printk("dt_gpio - Error! Could not set gpio up for interrupt\n");
            goto cleanup;
        }


	/* Creating procfs file */
	proc_file = proc_create("my-led", 0666, NULL, &fops);
	if(proc_file == NULL) {
		printk("procfs_test - Error creating /proc/my-led\n");
		gpiod_put(my_led);
		gpiod_put(my_interrupt);

		return -ENOMEM;
	}

	return 0;

cleanup:
    if (!IS_ERR_OR_NULL(my_led))
        gpiod_put(my_led);
    if (!IS_ERR_OR_NULL(my_interrupt))
        gpiod_put(my_interrupt);
    if (proc_file)
        proc_remove(proc_file);
    return ret;

}

/*
 * Remove function for the platform device.
 * This function is called when the device is being removed.
 * removes proc_file and frees GPIO
 */
static int dt_remove(struct platform_device *pdev) {
	printk("dt_gpio - Now I am in the remove function\n");
    free_irq(gpiod_to_irq(my_interrupt), NULL);
	gpiod_put(my_interrupt);
    gpiod_put(my_led);
    proc_remove(proc_file);
	return 0;
}

/*
 * Initialization function for the driver module.
 * This function is called when the module is loaded into the kernel.
 * It registers the platform driver with the kernel.
 */
static int __init my_init(void) {
	printk("dt_gpio - Loading the driver...\n");
	if(platform_driver_register(&my_driver)) {
		printk("dt_gpio - Error! Could not load driver\n");
		return -1;
	}
	return 0;
}

/*
 * Exit function for the driver module.
 * This function is called when the module is unloaded from the kernel.
 * It unregisters the platform driver.
 */
static void __exit my_exit(void) {
	printk("dt_gpio - Unload driver");
	platform_driver_unregister(&my_driver);
}

/*
 * Link open firmware ("of") to the before defined 
 * my_driver_ids array.
 */
MODULE_DEVICE_TABLE(of, my_driver_ids);

/*
 * Specify the entry and exit points for the driver.
 * These are executed when the driver is loaded or 
 * removed from the kernel.
 */
module_init(my_init);
module_exit(my_exit);

/*
 * Add licensing and other information to the driver
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("-");
MODULE_DESCRIPTION("-");

/*
 * To load the device tree overlay after compilation, use:
 * sudo dtoverlay testoverlay.dtbo
 */
