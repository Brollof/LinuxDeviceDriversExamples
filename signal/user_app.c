#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define SIGTX 44
#define REGISTER_UAPP       _IO('a', 'b')

void signal_handler(int sig)
{
    printf("Button signal received!\n");
}

int main(void)
{
    signal(SIGTX, signal_handler);

    printf("User app PID: %d\n", getpid());

    int fd = open("/dev/irq_signal", O_RDONLY);
    if (fd < 0)
    {
        printf("ERROR: opening device failed!\n");
        return 1;
    }

    if (ioctl(fd, REGISTER_UAPP, NULL) > 0)
    {
        printf("ERROR: registering app using ioctl failed!\n");
        return 1;
    }

    printf("Waiting for signal...\n");

    while (1)
    {
        sleep(1);
    }

    return 0;
}
