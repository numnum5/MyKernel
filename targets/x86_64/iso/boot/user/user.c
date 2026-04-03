#include "syscalls.h"

void _start()
{
    sys_write("Hello from user space!\n", 24);

    while (1);
}