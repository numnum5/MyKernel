
#include "x86_64/syscall.h"

static syscall_handler_t syscall_handlers[SYSCALL_NUM];

void init_syscalls(void)
{   
    for (uint8_t i = 0; i < SYSCALL_NUM; i++)
    {
        syscall_handlers[i] = syscall_default;
    }

    syscall_handlers[1] = syscall_write;
}

void syscall_default(syscall_reg_t * r)
{
    while(1)
    {
        asm volatile("hlt");
    }
}

void syscall_write(syscall_reg_t *r) 
{
    printf((char *)r->rdi);
    while(1);
}

void syscall_printf(syscall_reg_t *r) 
{
    printf((char *)r->rdi);
    while(1);
}

void syscall_handler(syscall_reg_t* r)
{
    if (r->rax < (uint64_t) SYSCALL_NUM) 
    {
        syscall_handlers[r->rax](r);
    }
}
