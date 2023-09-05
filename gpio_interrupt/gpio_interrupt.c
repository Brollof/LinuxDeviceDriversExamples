#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brolaf");
MODULE_DESCRIPTION("GPIO interrupt example");

#define PREFIX          "[MY_DEV] "
#define myprintk(...)   printk(PREFIX __VA_ARGS__)

static int irq_number = 0;

static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
    myprintk("IRQ triggered\n");
    return IRQ_HANDLED;
}

static int __init ModuleInit(void)
{
    myprintk("Hello from kernel\n");

    if (gpio_request(4, "gpio-4-int"))
    {
        myprintk("Cannot request GPIO 4!\n");
        return -1;
    }

    if (gpio_direction_input(4))
    {
        gpio_free(4);
        myprintk("Cannot set direction!\n");
        return -1;
    }

    irq_number = gpio_to_irq(4);
    myprintk("IRQ number: %d\n", irq_number);

    if (request_irq(irq_number, gpio_irq_handler, IRQF_TRIGGER_FALLING, "my_gpio_irq", NULL) != 0)
    {
        gpio_free(4);
        myprintk("Cannot request IRQ!\n");
        return -1;
    }


    myprintk("Modul init OK\n");

    return 0;
}

static void __exit ModuleExit(void)
{
    myprintk("Goodbye from kernel\n");
    free_irq(irq_number, NULL);
    gpio_free(4);

}

module_init(ModuleInit);
module_exit(ModuleExit);
