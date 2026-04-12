#include <stdint.h>
#include "print.h"
#define IA32_EFER   0xC0000080
#define IA32_STAR   0xC0000081
#define IA32_LSTAR  0xC0000082
#define IA32_FMASK  0xC0000084

extern void syscall_entry();

uint64_t read_msr(uint64_t msr) 
{
    uint64_t rax, rdx;
    asm("rdmsr" : "=a" (rax), "=d" (rdx) : "c"(msr));

    return (rdx << 32) | rax;
}

void write_msr(uint64_t msr, uint64_t data) 
{
    uint64_t rax = data & 0xFFFFFFFF;
    uint64_t rdx = data >> 32;
    asm("wrmsr" :: "a" (rax), "d" (rdx), "c"(msr));
}

void init_scheduler_msr() 
{
    write_msr(0xC0000081, read_msr(0xC0000081) | ((uint64_t) 0x8 << 32));
    write_msr(0xC0000081, read_msr(0xC0000081) | ((uint64_t) 0x18 << 48));
    write_msr(0xC0000082, (uint64_t) syscall_entry); // Start execution at the syscall stub when a syscall occurs
    write_msr(0xC0000084, 0); 
    write_msr(0xC0000080, read_msr(0xC0000080) | 1); 
}