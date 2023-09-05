#include <linux/module.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brolaf");
MODULE_DESCRIPTION("Parameters usage demo");

#define PREFIX          "[MY_DEV] "
#define myprintk(...)   printk(PREFIX __VA_ARGS__)

static unsigned int gpio_nr = 4;
static char *device_name = "testdevice";

module_param(gpio_nr, uint, S_IRUGO);
module_param(device_name, charp, S_IRUGO);

MODULE_PARM_DESC(gpio_nr, "Number of GPIO to use");
MODULE_PARM_DESC(device_name, "Device name");

static int __init ModuleInit(void)
{
    myprintk("Hello from kernel\n");

    myprintk("gpio_nr: %d\n", gpio_nr);
    myprintk("device_name: %s\n", device_name);

    return 0;
}

static void __exit ModuleExit(void)
{
    myprintk("Goodbye from kernel\n");
}

module_init(ModuleInit);
module_exit(ModuleExit);

// Usage:
// sudo insmod my_params.ko device_name=blabla gpio_nr=15
