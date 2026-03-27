#include <stdint.h>
#include "print.h"
#define IA32_EFER   0xC0000080
#define IA32_STAR   0xC0000081
#define IA32_LSTAR  0xC0000082
#define IA32_FMASK  0xC0000084

extern void syscall_entry();


static inline void wrmsr(uint32_t msr, uint64_t value) {
    uint32_t low = value & 0xFFFFFFFF;
    uint32_t high = value >> 32;
    asm volatile (
        "wrmsr"
        :
        : "c"(msr), "a"(low), "d"(high)
        : "memory"
    );
}

void init_syscall_interface() {
    // 1. Enable SCE in EFER
    uint32_t low, high;
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(IA32_EFER));
    wrmsr(IA32_EFER, ((uint64_t)high << 32) | (low | 1));

    // 2. Set syscall entry point
    // wrmsr(IA32_LSTAR, (uint64_t)syscall_entry);

    // 3. Set STAR (CS selectors)
    uint64_t star = ((uint64_t)0x1B << 48) | ((uint64_t)0x08 << 32);
    wrmsr(IA32_STAR, star);

    // 4. Set flags to clear on entry
    wrmsr(IA32_FMASK, 0x202);
}


uint64_t syscall1(uint64_t number, uint64_t arg1) 
{
    uint64_t ret;
    asm volatile(
        "syscall"
        : "=a"(ret)             // output: return value in RAX
        : "a"(number),           // input: syscall number in RAX
          "D"(arg1)              // input: 1st argument in RDI
        : "rcx", "r11", "memory" // clobbered registers
    );
    return ret;
}