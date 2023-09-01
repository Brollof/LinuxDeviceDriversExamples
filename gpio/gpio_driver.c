#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brolaf");
MODULE_DESCRIPTION("GPIO Driver");

#define DEVICE_NAME     "my_dev"
#define CLASS_NAME      "my_class"
#define DEV_COUNT       1

#define PREFIX          "[MY_DEV] "
#define myprintk(...)   printk(PREFIX __VA_ARGS__)

static dev_t device_number = 0;
static int major = 0;
static int minor = 0;

static struct class *my_class = NULL;
static struct device *my_devices = NULL;
static struct cdev devs;

static char buffer[256] = {0};

static int driver_open(struct inode *dev_file, struct file *instance);
static int driver_close(struct inode *dev_file, struct file *instance);
static ssize_t driver_read(struct file *dev_file, char *user_buf, size_t count, loff_t *offset);
static ssize_t driver_write(struct file *dev_file, const char *user_buf, size_t count, loff_t *offset);
static struct file_operations fops =
{
    .owner = THIS_MODULE,
    .open = driver_open,
    .release = driver_close,
    .read = driver_read,
    .write = driver_write
};

static int driver_open(struct inode *dev_file, struct file *instance)
{
    myprintk("Open called\n");
    return 0;
}

static int driver_close(struct inode *dev_file, struct file *instance)
{
    myprintk("Close called\n");
    return 0;
}

static ssize_t driver_read(struct file *dev_file, char *user_buf, size_t count, loff_t *offset)
{
    myprintk("Read called\n");

    char tmp[3] = " \n";
    int to_copy = min(count, sizeof(tmp));

    tmp[0] = gpio_get_value(4) + '0';
    int not_copied = copy_to_user(user_buf, tmp, to_copy);

    return to_copy - not_copied;
}

static ssize_t driver_write(struct file *dev_file, const char *user_buf, size_t count, loff_t *offset)
{
    myprintk("Write called\n");
    char value = '0';
    int not_copied = copy_from_user(&value, user_buf, 1);

    switch (value)
    {
        case '0':
            gpio_set_value(4, 0);
            break;
        case '1':
            gpio_set_value(4, 1);
            break;
        default:
            myprintk("Wrong gpio value!\n");
            break;
    }

    return 1 - not_copied;
}

static int __init ModuleInit(void)
{
    myprintk("Hello from kernel\n");

    int ret = alloc_chrdev_region(&device_number, 0, DEV_COUNT, DEVICE_NAME);
    if (ret < 0)
    {
        myprintk("Device number not allocated, error: %d\n", ret);
        return -1;
    }

    major = MAJOR(device_number);
    minor = MINOR(device_number);

    myprintk("Device number - Major: %d, Minor: %d\n", major, minor);

    cdev_init(&devs, &fops);
    devs.owner = THIS_MODULE;
    cdev_add(&devs, device_number, 1);

    my_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(my_class))
    {
        myprintk("Failed to create class! Error: %d\n", (int)PTR_ERR(my_class));
        goto class_error;
    }

    my_devices = device_create(my_class, NULL, device_number, NULL, DEVICE_NAME);
    if (IS_ERR(my_devices))
    {
        myprintk("Failed to create device! Error: %d\n", (int)PTR_ERR(my_devices));
        goto device_error;
    }

	/* GPIO 4 init */
	if(gpio_request(4, "rpi-gpio-4")) {
		printk("Can not allocate GPIO 4\n");
		goto gpio_error;
	}

	/* Set GPIO 4 direction */
	if(gpio_direction_output(4, 0)) {
		printk("Can not set GPIO 4 to output!\n");
		goto gpio_direction_error;
	}

    gpio_export(4, false);

    myprintk("Init OK!\n");
    return 0;

gpio_direction_error:
    gpio_free(4);
gpio_error:
    device_destroy(my_class, device_number);
device_error:
    class_destroy(my_class);
class_error:
    cdev_del(&devs);
    unregister_chrdev_region(device_number, DEV_COUNT);

    return -1;
}

static void __exit ModuleExit(void)
{
    myprintk("Goodbye from kernel\n");
    gpio_unexport(4);
    gpio_free(4);
    device_destroy(my_class, device_number);
    class_destroy(my_class);
    cdev_del(&devs);
    unregister_chrdev_region(device_number, DEV_COUNT);
}

module_init(ModuleInit);
module_exit(ModuleExit);
