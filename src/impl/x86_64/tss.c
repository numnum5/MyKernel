#include "print.h"
#include "x86_64/tss.h"

static TSS tss = {0};

extern uint8_t stack_tss_top[];
extern uint8_t stack_tss_bottom[];
extern uint8_t gdt64[];

void tss_init(void) 
{
    printf("bottom tss: %x\n", (uint64_t) stack_tss_bottom);
    printf("top tss: %x\n", (uint64_t) stack_tss_top);
    uint32_t limit = sizeof(TSS);

    tss.rsp0 = (uint64_t) stack_tss_top;
    tss.iomap_base = sizeof(TSS);

    TSS_Descriptor  * descriptor = &gdt64[0x28];

    uint64_t tss_addr = (uint64_t)&tss;

    descriptor->limit_low = limit & 0xFFFF;
    descriptor->base_low = tss_addr & 0xFFFF;
    descriptor->base_mid = (tss_addr >> 16) & 0xFF;
    descriptor->type = 0x89; // Ring 0
    descriptor->limit_high_flags = ((limit >> 16) & 0x0F);
    descriptor->base_high = (tss_addr >> 24) & 0xFF;
    descriptor->base_upper = (tss_addr >> 32) & 0xFFFFFFFF;
    descriptor->reserved = 0;

    asm volatile("ltr %0" ::"r"(0x28));
}