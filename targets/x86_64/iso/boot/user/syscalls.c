#include <stdint.h>

#define SYS_WRITE 1
#define SYS_EXIT  12

long sys_write(const char *buf, uint64_t len)
{
    long ret;
    asm volatile (
        "mov $1, %%rax;"
        "mov %1, %%rdi;"
        "mov %2, %%rsi;"
        "syscall;"
        "mov %%rax, %0;"
        : "=r"(ret)
        : "r"(buf), "r"(len)
        : "rax","rdi","rsi","rcx","r11"
    );
    return ret;
}