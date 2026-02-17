#include "x86_64/util.h"
#include "x86_64/interrupts.h"
#include "print.h"


void set_idt_gate(uint8_t vector, uint64_t isr_addr, uint16_t selector, uint8_t type);

struct idt_entry {
	uint16_t offset_low;
	uint16_t selector;
	uint8_t  ist;
	uint8_t  type;
	uint16_t offset_mid;
	uint32_t offset_high;
	uint32_t reserved;
} __attribute__((packed));


struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

struct idt_entry idt_entries[256];
struct idt_ptr idt_ptr2;

extern void timer_interrupt_handler_wrapped();

volatile uint32_t pit_ticks = 0;

void timer_interrupt_handler() {

    pit_ticks++;

    if (pit_ticks >= 18) {
        pit_ticks = 0;
            print_str("interrupt called\n");
    }

    out_port_B(0x20,0x20);
	// pic_eoi_master();
}

void idt_initv2()
{
    out_port_B(0x20, 0x11);
    out_port_B(0xA0, 0x11);

    out_port_B(0x21, 0x20);
    out_port_B(0xA1, 0x28);

    out_port_B(0x21,0x04);
    out_port_B(0xA1,0x02);

    out_port_B(0x21, 0x01);
    out_port_B(0xA1, 0x01);

    out_port_B(0x21, 0x0);
    out_port_B(0xA1, 0x0);

    idt_ptr2.limit = (sizeof(struct idt_entry) * 256) - 1;
	idt_ptr2.base = (uint64_t) &idt_entries;

    set_idt_gate(32, (uint64_t) timer_interrupt_handler_wrapped, 0x08, 0x8E);
    
    asm volatile("lidt %0"
                :
                : "m"(idt_ptr2));

    asm volatile("sti");
}


void set_idt_gate(uint8_t vector, uint64_t isr_addr, uint16_t selector, uint8_t type) {
    struct idt_entry * entry = &idt_entries[vector];

    entry->offset_low = (uint16_t) (isr_addr >> 0);
    entry->selector = selector;
    entry->ist = 0;
    entry->type = type;
    entry->offset_mid = (uint16_t) (isr_addr >> 16);
    entry->offset_high = (uint32_t) (isr_addr >> 32);
    entry->reserved = 0;
}