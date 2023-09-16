#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "../my_ioctl.h"

int main(void)
{
    int dev = open("/dev/my_dev", O_RDONLY);
    if (dev == -1)
    {
        printf("Open failed!\n");
        return -1;
    }

    struct compute data = {0};
    data.a = 55;
    data.b = 10;

    printf("Printking last result in kernel space...\n");
    ioctl(dev, CMD_PRINT_LAST, NULL);

    ioctl(dev, CMD_ADD, &data);
    printf("Add: %d + %d = %d\n", data.a, data.b, data.result);

    ioctl(dev, CMD_MUL, &data);
    printf("Mul: %d * %d = %d\n", data.a, data.b, data.result);

    int last = 0;
    ioctl(dev, CMD_READ_LAST, &last);
    printf("Last result read: %d\n", last);

    printf("Open OK, closing...\n");
    close(dev);

    return 0;
}