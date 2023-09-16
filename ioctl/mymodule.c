#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include "my_ioctl.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brolaf");
MODULE_DESCRIPTION("Hello world");

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

static int driver_open(struct inode *dev_file, struct file *instance);
static int driver_close(struct inode *dev_file, struct file *instance);
static ssize_t driver_read(struct file *dev_file, char *user_buf, size_t count, loff_t *offset);
static ssize_t driver_write(struct file *dev_file, const char *user_buf, size_t count, loff_t *offset);
static long int my_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static struct file_operations fops =
{
    .owner = THIS_MODULE,
    .open = driver_open,
    .release = driver_close,
    .read = driver_read,
    .write = driver_write,
    .unlocked_ioctl = my_ioctl
};

static int last_result = 0;

static long int my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct compute data = {0};

    switch (cmd)
    {
        case CMD_ADD:
            if (copy_from_user(&data, (struct compute *)arg, sizeof(struct compute)) > 0)
            {
                myprintk("IOCTL CMD_ADD - couldn't copy from user\n");
                return -1;
            }

            data.result = data.a + data.b;
            last_result = data.result;

            if (copy_to_user((struct compute *)arg, &data, sizeof(struct compute)) > 0)
            {
                myprintk("IOCTL CMD_ADD - couldn't copy to user\n");
                return -1;
            }
            myprintk("IOCTL CMD_ADD - finished!\n");
            break;

        case CMD_MUL:
            if (copy_from_user(&data, (struct compute *)arg, sizeof(struct compute)) > 0)
            {
                myprintk("IOCTL CMD_MUL - couldn't copy from user\n");
                return -1;
            }

            data.result = data.a * data.b;
            last_result = data.result;

            if (copy_to_user((struct compute *)arg, &data, sizeof(struct compute)) > 0)
            {
                myprintk("IOCTL CMD_MUL - couldn't copy to user\n");
                return -1;
            }
            myprintk("IOCTL CMD_MUL - finished!\n");
            break;

        case CMD_READ_LAST:
            if (copy_to_user((int *)arg, &last_result, sizeof(last_result)) > 0)
            {
                myprintk("IOCTL CMD_READ_LAST - couldn't copy to user\n");
                return -1;
            }
            break;

        case CMD_PRINT_LAST:
            myprintk("Last result: %d\n", last_result);
            break;
    }

    return 0;
}

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

    return 0;
}

static ssize_t driver_write(struct file *dev_file, const char *user_buf, size_t count, loff_t *offset)
{
    myprintk("Write called\n");

    return count;
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

    myprintk("Init OK!\n");
    return 0;

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
    device_destroy(my_class, device_number);
    class_destroy(my_class);
    cdev_del(&devs);
    unregister_chrdev_region(device_number, DEV_COUNT);
}

module_init(ModuleInit);
module_exit(ModuleExit);
