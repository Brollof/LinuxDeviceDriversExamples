#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(void)
{
    int dev = open("/dev/mydevice", O_RDONLY);
    if (dev == -1)
    {
        printf("Open failed!\n");
        return -1;
    }

    printf("Open OK, closing...\n");
    close(dev);

    return 0;
}