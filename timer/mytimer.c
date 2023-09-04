#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/jiffies.h>
#include <linux/timer.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brolaf");
MODULE_DESCRIPTION("Timer example");

#define PREFIX              "[MY_DEV] "
#define myprintk(...)       printk(PREFIX __VA_ARGS__)

#define TIMER_PERIOD_MS     500
#define LED_GPIO_PIN        4

static struct timer_list my_timer = {0};

static void timer_callback(struct timer_list *list)
{
    gpio_set_value(LED_GPIO_PIN, !gpio_get_value(LED_GPIO_PIN));
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(TIMER_PERIOD_MS));
}

static int __init ModuleInit(void)
{
    myprintk("Hello from kernel\n");

    if (gpio_request(LED_GPIO_PIN, "LED-PIN"))
    {
        myprintk("Cannot allocatate GPIO %d\n", LED_GPIO_PIN);
        return -1;
    }

    if (gpio_direction_output(LED_GPIO_PIN, 0))
    {
        myprintk("Cannot set GPIO direction!\n");
        gpio_free(LED_GPIO_PIN);
        return -1;
    }

    gpio_set_value(LED_GPIO_PIN, 0);

    timer_setup(&my_timer, timer_callback, 0);
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(TIMER_PERIOD_MS));

    return 0;
}

static void __exit ModuleExit(void)
{
    myprintk("Goodbye from kernel\n");
    gpio_set_value(LED_GPIO_PIN, 0);
    gpio_free(LED_GPIO_PIN);
    del_timer(&my_timer);
}

module_init(ModuleInit);
module_exit(ModuleExit);
