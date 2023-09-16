#ifndef _MY_IOCTL_H
#define _MY_IOCTL_H

struct compute
{
    int a;
    int b;
    int result;
};

#define CMD_ADD         _IOWR('a', 'a', struct compute *)
#define CMD_MUL         _IOWR('a', 'b', struct comput *)
#define CMD_READ_LAST   _IOR('a', 'c', int *)
#define CMD_PRINT_LAST  _IO('a', 'd')

#endif