#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brolaf");
MODULE_DESCRIPTION("Hello world");

#define MAJOR_NUMBER    101
#define FIRST_DEV       MKDEV(MAJOR_NUMBER, 0)
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

    int to_copy = strlen(buffer);

    if (*offset >= to_copy)
        return 0;

    if (copy_to_user(user_buf, buffer, to_copy))
        return -EFAULT;

    *offset += to_copy;
    return to_copy;
}

static ssize_t driver_write(struct file *dev_file, const char *user_buf, size_t count, loff_t *offset)
{
    myprintk("Write called\n");

    if (count >= sizeof(buffer))
        return -EINVAL;

    if (copy_from_user(buffer, user_buf, count))
        return -EFAULT;

    buffer[count] = 0;

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

    return 0;
}

static void __exit ModuleExit(void)
{
    myprintk("Goodbye from kernel\n");
    device_destroy(my_class, device_number);
    class_destroy(my_class);
    cdev_del(&devs);
    unregister_chrdev_region(MKDEV(MAJOR_NUMBER, 0), DEV_COUNT);
}

module_init(ModuleInit);
module_exit(ModuleExit);
