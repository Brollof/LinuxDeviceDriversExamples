#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/i2c.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brolaf");
MODULE_DESCRIPTION("Light sensor I2C driver");

#define DEVICE_NAME                     "light-sensor"
#define CLASS_NAME                      "light-sensor"
#define DEV_COUNT                       1

#define PREFIX                          "[MY_DEV] "
#define myprintk(...)                   printk(PREFIX __VA_ARGS__)

#define I2C_BUS                         1
#define SLAVE_NAME                      "LIGHT_SENSOR"
#define LIGHT_SENSOR_SLAVE_ADDR         0x39

static struct i2c_adapter *ls_i2c_adapter = NULL;
static struct i2c_client *ls_i2c_client = NULL;

static const struct i2c_device_id ls_id[] =
{
    { SLAVE_NAME, 0},
    {}
};

static struct i2c_driver ls_driver =
{
    .driver =
    {
        .name = SLAVE_NAME,
        .owner = THIS_MODULE
    }
};

static const struct i2c_board_info ls_i2c_board_info =
{
    I2C_BOARD_INFO(SLAVE_NAME, LIGHT_SENSOR_SLAVE_ADDR)
};

static dev_t device_number = 0;
static int major = 0;
static int minor = 0;

static struct class *ls_class = NULL;
static struct device *ls_device = NULL;
static struct cdev dev;

static int driver_open(struct inode *dev_file, struct file *instance);
static int driver_close(struct inode *dev_file, struct file *instance);
static ssize_t driver_read(struct file *dev_file, char *user_buf, size_t count, loff_t *offset);

static struct file_operations fops =
{
    .owner = THIS_MODULE,
    .open = driver_open,
    .release = driver_close,
    .read = driver_read
};


/* -------------------------------------------------------------------------- */

#define CMD                             (1 << 7)
#define T_READ                          (1 << 6)

#define REG_CONTROL                     0x00
#define REG_TIMING                      0x01
#define REG_ANALOG                      0x07
#define REG_ID                          0x12
#define REG_DATA0_LOW                   0x14
#define REG_DATA0_HIGH                  0x15
#define REG_DATA1_LOW                   0x16
#define REG_DATA1_HIGH                  0x17

#define INT_TIME_100_MS                 0b11011011
#define INT_TIME_200_MS                 0b10110110
#define INT_TIME_400_MS                 0b01101100
#define INT_TIME_700_MS                 0x01

#define GAIN_1X                         0x00
#define GAIN_8X                         0x01
#define GAIN_16X                        0x02
#define GAIN_111X                       0x03

static uint8_t ls_read_reg(uint8_t reg)
{
    return i2c_smbus_read_byte_data(ls_i2c_client, CMD | T_READ | reg);
}

static void ls_write_reg(uint8_t reg, uint8_t value)
{
    i2c_smbus_write_byte_data(ls_i2c_client, CMD | reg, value);
}

static void ls_power_on(void)
{
    ls_write_reg(REG_CONTROL, 0x03); // ADC_EN, Power ON
}

static void ls_power_off(void)
{
    ls_write_reg(REG_CONTROL, 0);
}

static void ls_init(void)
{
    ls_write_reg(REG_TIMING, INT_TIME_400_MS);
    ls_write_reg(REG_ANALOG, GAIN_16X);
    ls_power_on();
}

uint32_t ls_get(void)
{
    uint16_t ch0 = (ls_read_reg(REG_DATA0_HIGH) << 8) + ls_read_reg(REG_DATA0_LOW);
    uint16_t ch1 = (ls_read_reg(REG_DATA1_HIGH) << 8) + ls_read_reg(REG_DATA1_LOW);

    myprintk("CH0: %d\n", ch0);
    myprintk("CH1: %d\n", ch1);

    uint32_t ch0_scale = 65536 >> 4;
    uint32_t ch1_scale = ch0_scale;

    ch0 = (ch0 * ch0_scale) >> 16;
    ch1 = (ch1 * ch1_scale) >> 16;

    uint32_t ratio1 = (ch1 << (10)) / ch0;
    uint32_t ratio = (ratio1 + 1) >> 1;

    return ratio;
}
/* -------------------------------------------------------------------------- */

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

    char buf[32] = {0};
    uint32_t v = ls_get();

    sprintf(buf, "%d\n", v);

    int to_copy = min(strlen(buf) + 1, count);
    int not_copied = copy_to_user(user_buf, buf, to_copy);

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

    cdev_init(&dev, &fops);
    dev.owner = THIS_MODULE;
    cdev_add(&dev, device_number, 1);

    ls_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(ls_class))
    {
        myprintk("Failed to create class! Error: %d\n", (int)PTR_ERR(ls_class));
        goto class_error;
    }

    ls_device = device_create(ls_class, NULL, device_number, NULL, DEVICE_NAME);
    if (IS_ERR(ls_device))
    {
        myprintk("Failed to create device! Error: %d\n", (int)PTR_ERR(ls_device));
        goto device_error;
    }

    ls_i2c_adapter = i2c_get_adapter(I2C_BUS);
    if (ls_i2c_adapter != NULL)
    {
        ls_i2c_client = i2c_new_client_device(ls_i2c_adapter, &ls_i2c_board_info);
        if (ls_i2c_client != NULL)
        {
            if (i2c_add_driver(&ls_driver) != -1)
            {
                ret = 0;
            }
            else
            {
                myprintk("Can't add driver...\n");
                goto i2c_init_error;
            }
        }
        else
        {
            goto i2c_init_error;
        }
        i2c_put_adapter(ls_i2c_adapter);
    }
    else
    {
        goto i2c_init_error;
    }

    uint8_t id = ls_read_reg(REG_ID);

    myprintk("Light sensor driver added!\n");

    myprintk("PARTNO: 0x%02X\n", (id & 0xF0) >> 4);
    myprintk("REVNO: 0x%02X\n", id & 0x0F);

    ls_init();

    myprintk("Module init OK!\n");

    return 0;

i2c_init_error:
    device_destroy(ls_class, device_number);
device_error:
    class_destroy(ls_class);
class_error:
    cdev_del(&dev);
    unregister_chrdev_region(device_number, DEV_COUNT);

    return -1;
}

static void __exit ModuleExit(void)
{
    myprintk("Goodbye from kernel\n");
    ls_power_off();
    i2c_unregister_device(ls_i2c_client);
    i2c_del_driver(&ls_driver);
    device_destroy(ls_class, device_number);
    class_destroy(ls_class);
    cdev_del(&dev);
    unregister_chrdev_region(device_number, DEV_COUNT);
}

module_init(ModuleInit);
module_exit(ModuleExit);
