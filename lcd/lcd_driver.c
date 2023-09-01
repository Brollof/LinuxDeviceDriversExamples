#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include "lcd_driver.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brolaf");
MODULE_DESCRIPTION("LCD Driver");

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

static char lcd_buf[CHAR_NUM + 1] = {0};

static int driver_open(struct inode *dev_file, struct file *instance);
static int driver_close(struct inode *dev_file, struct file *instance);
static ssize_t driver_write(struct file *dev_file, const char *user_buf, size_t count, loff_t *offset);

static struct file_operations fops =
{
    .owner = THIS_MODULE,
    .open = driver_open,
    .release = driver_close,
    .write = driver_write
};

typedef struct
{
    const char *name;
    uint8_t pin;
} mygpio_t;

typedef enum
{
    RS, RW, EN,
    D0, D1, D2, D3, D4, D5, D6, D7
} gpio_type_t;

static const mygpio_t gpios[] =
{
    [RS] = {"LCD_RS", 18},
    [RW] = {"LCD_RW", 15},
    [EN] = {"LCD_EN", 14},
    [D0] = {"LCD_D0", 26},
    [D1] = {"LCD_D1", 19},
    [D2] = {"LCD_D2", 13},
    [D3] = {"LCD_D3",  6},
    [D4] = {"LCD_D4",  5},
    [D5] = {"LCD_D5",  0},
    [D6] = {"LCD_D6", 11},
    [D7] = {"LCD_D7",  9}
};

static const int gpios_count = sizeof(gpios) / sizeof(gpios[0]);

/* Generate pulse on the enable signal */
static void lcd_enable(void)
{
    LCD_EN(1);
    msleep(5);
    LCD_EN(0);
}

/* Set the 8-bit data bus*/
static void lcd_send_byte(uint8_t data)
{
    myprintk("Sending byte: 0x%02X\n", data);
    for (int i = 0; i < 8; i++)
    {
        gpio_set_value(gpios[i+3].pin, (data & (1 << i)) >> i);
    }
    lcd_enable();
    msleep(5);
}

/* Send a command to the LCD */
static void lcd_command(uint8_t data)
{
    LCD_RS(0);
    lcd_send_byte(data);
}

/* Send a data to the LCD */
static void lcd_data(uint8_t data)
{
    LCD_RS(1);
    lcd_send_byte(data);
}

/* Send whole string at given line */
static void lcd_send_text(const char* txt, uint8_t n)
{
    if (n == 1)
        lcd_command(LCD_LINE_1);
    else if (n == 2)
        lcd_command(LCD_LINE_2);

    for (size_t i = 0; i < strlen(txt); i++)
    {
        lcd_data(txt[i]);
    }
}

/* Initialize the LCD */
static void lcd_init(void)
{
    lcd_command(0x38); // 8bits data, 2 lines
    lcd_command(0x0C); // disp on, cursor off, cursor blinking off
    lcd_command(LCD_CLEAR);

    LCD_RW(0);         // only write mode

    lcd_send_text("Hello", 1);
    lcd_send_text("World", 2);
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

static ssize_t driver_write(struct file *dev_file, const char *user_buf, size_t count, loff_t *offset)
{
    myprintk("Write called\n");

    int to_copy = min(count, sizeof(lcd_buf));
    int not_copied = copy_from_user(lcd_buf, user_buf, to_copy);

    lcd_command(LCD_CLEAR);

    for (int i = 0; i < to_copy; i++)
    {
        if (0x20 <= lcd_buf[i] && lcd_buf[i] <= 0x7F) // allow only printable characters
        {
            lcd_data(lcd_buf[i]);
        }
    }

    return to_copy - not_copied;
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

    int i = 0;
    for (i = 0; i < gpios_count; i++)
    {
        if (gpio_request(gpios[i].pin, gpios[i].name))
        {
            myprintk("Error initializing GPIO: %d, name: %s\n", gpios[i].pin, gpios[i].name);
            goto gpio_request_error;
        }
    }

    for (i = 0; i < gpios_count; i++)
    {
        if (gpio_direction_output(gpios[i].pin, 0))
        {
            myprintk("Error initializing GPIO: %d, name: %s\n", gpios[i].pin, gpios[i].name);
            goto gpio_direction_error;
        }
    }

    lcd_init();

    myprintk("Module init OK!\n");
    return 0;

gpio_direction_error:
    i = gpios_count - 1;
gpio_request_error:
    for (; i >= 0; i--)
    {
        gpio_free(gpios[i].pin);
    }
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
    for (int i = 0; i < gpios_count; i++)
    {
        gpio_set_value(gpios[i].pin, 0);
        gpio_free(gpios[i].pin);
    }
    device_destroy(my_class, device_number);
    class_destroy(my_class);
    cdev_del(&devs);
    unregister_chrdev_region(device_number, DEV_COUNT);
}

module_init(ModuleInit);
module_exit(ModuleExit);
