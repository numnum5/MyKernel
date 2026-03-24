#include <stdint.h>
extern uint8_t stack_tss_top[];
extern uint8_t stack_tss_bottom[];
extern uint8_t gdt64[];

typedef struct __attribute__((packed)) {
    uint32_t reserved0;
    uint64_t rsp0;      // stack for ring 0
    uint64_t rsp1;      // stack for ring 1 (optional)
    uint64_t rsp2;      // stack for ring 2 (optional)
    uint64_t reserved1;
    uint64_t ist1;      // interrupt stack table entries
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} TSS;

typedef struct __attribute__((packed)) {
    uint16_t limit_low;       // bits 0-15 of TSS limit
    uint16_t base_low;        // bits 0-15 of TSS base
    uint8_t  base_mid;        // bits 16-23 of TSS base
    uint8_t  type;            // Type (0x9), S=0, DPL, P
    uint8_t  limit_high_flags; // bits 16-19 of limit + flags (G, AVL)
    uint8_t  base_high;       // bits 24-31 of TSS base
    uint32_t base_upper;      // bits 32-63 of TSS base
    uint32_t reserved;        // must be zero
} TSS_Descriptor;


static TSS tss = {0};

void init_TSS(void) {
    uint32_t limit = sizeof(TSS);

    tss.rsp0 = (uint64_t) stack_tss_top;
    tss.iomap_base = sizeof(TSS);

    TSS_Descriptor  * descriptor = &gdt64[0x28];

    uint64_t tss_addr = (uint64_t)&tss;

    descriptor->limit_low = limit & 0xFFFF;
    descriptor->base_low = tss_addr & 0xFFFF;
    descriptor->base_mid = (tss_addr >> 16) & 0xFF;
    descriptor->type = 0x89; // Present, Executable (TSS type), Ring 0
    descriptor->limit_high_flags = ((limit >> 16) & 0x0F);
    descriptor->base_high = (tss_addr >> 24) & 0xFF;
    descriptor->base_upper = (tss_addr >> 32) & 0xFFFFFFFF;
    descriptor->reserved = 0;

    asm volatile("ltr %0" ::"r"(0x28));
}