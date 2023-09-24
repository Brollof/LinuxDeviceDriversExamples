#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/sched/signal.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brolaf");
MODULE_DESCRIPTION("Sending signal to the user space example");

#define PREFIX          "[MY_DEV] "
#define myprintk(...)   printk(PREFIX __VA_ARGS__)

#define MYMAJOR             64
#define REGISTER_UAPP       _IO('a', 'b')
#define SIGNAL_NUM          44

static int irq_number = 0;
static struct task_struct *task = NULL;

static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
    myprintk("IRQ triggered\n");

    if (task != NULL)
    {
        struct siginfo info =
        {
            .si_signo = SIGNAL_NUM,
            .si_code = SI_QUEUE
        };

        /* Send signal to the user space */
        if (send_sig_info(SIGNAL_NUM, (struct kernel_siginfo *)&info, task) < 0)
        {
            myprintk("Error sending signal!\n");
        }
    }

    return IRQ_HANDLED;
}

static int my_close(struct inode *device_file, struct file *instance)
{
    if (task != NULL)
    {
        task = NULL;
    }

    return 0;
}

static long int my_iocontrol(struct file *file, unsigned int cmd, unsigned long arg)
{
    if (cmd == REGISTER_UAPP)
    {
        task = get_current();
        myprintk("gpio_irq_signal: Userspace with PID %d is registered\n", task->pid);
    }

    return 0;
}

static struct file_operations fops =
{
    .owner = THIS_MODULE,
    .release = my_close,
    .unlocked_ioctl = my_iocontrol
};

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

	if (register_chrdev(MYMAJOR, "gpio_irq_signal", &fops) < 0) {
		myprintk("Cannot register device number: %d!\n", MYMAJOR);
		free_irq(irq_number, NULL);
		gpio_free(4);
	}

    myprintk("Modul init OK\n");

    return 0;
}

static void __exit ModuleExit(void)
{
    myprintk("Goodbye from kernel\n");
    free_irq(irq_number, NULL);
    gpio_free(4);
    unregister_chrdev(MYMAJOR, "gpio_irq_signal");
}

module_init(ModuleInit);
module_exit(ModuleExit);

// sudo mknod /dev/irq_signal c 64 0

/*
 * To automatically load kenerl module:
 * 1. sudo make install
 * 2. vim /etc/modules
 * 3. sudo depmod
 * 4. reboot
 */
