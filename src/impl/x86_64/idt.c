#include "x86_64/idt.h"

struct idt_entry idt_entries[256];
struct idt_ptr idt_ptr;
extern void scheduler_yield(void);

void default_handler(uint64_t * sp)
{
    printf("FAULT");
    while(1);
}

void gpf_handler_c(interrupt_frame_t *f ) 
{
    printf("INT: %d ERR: 0x%x\n", f->int_no, f->err_code);
    printf("RIP: 0x%x RSP: 0x%x\n", f->rip, f->rsp);

    uint64_t * rsp = (uint64_t *)(f->rsp - 5 * 8);

    printf("rip: %x\n", rsp[0]);
    printf("rip: %x\n", rsp[1]);
    printf("rip: %x\n", rsp[2]);
    printf("rip: %x\n", rsp[3]);   
    printf("rip: %x\n", rsp[4]);

    while (1) 
    {
        asm volatile("hlt");
    }
}


void idt_init(void)
{
    out_port_B(0x20, 0x11); // ICW1 master
    out_port_B(0xA0, 0x11); // ICW1 slave

    out_port_B(0x21, 0x20); // ICW2 master → offset 32
    out_port_B(0xA1, 0x28); // ICW2 slave  → offset 40

    out_port_B(0x21,0x04);  // ICW3 master (slave at IRQ2)
    out_port_B(0xA1,0x02);  // ICW3 slave (cascade identity)

    out_port_B(0x21, 0x01); // ICW4 master
    out_port_B(0xA1, 0x01); // ICW4 slave

    
    // out_port_B(0x21, 0xFF); // 11111110 → enable IRQ0 only
    out_port_B(0x21, 0xFE); // 11111110 → enable IRQ0 only
    out_port_B(0xA1, 0xFF); // keep slave masked for now

    idt_ptr.limit = (sizeof(struct idt_entry) * 256) - 1;
	idt_ptr.base = (uint64_t) &idt_entries;

// Before setting specific gates:
    extern void* isr_table[32];

    for (uint16_t i = 0; i < 32; i++) 
    {
        set_idt_gate(i, isr_table[i], 0x08, 0x8E);
    }

    set_idt_gate(32, (uint64_t) scheduler_yield, 0x08, 0x8E);


    for (uint16_t i = 33; i < 256; i++) 
    {
        set_idt_gate(i, (uint64_t) default_handler_wrapped, 0x08, 0x8E);
    }


    asm volatile("lidt %0"
                :
                : "m"(idt_ptr));

    asm volatile("sti");
}

void set_idt_gate(uint8_t vector, uint64_t isr_addr, uint16_t selector, uint8_t type) 
{
    struct idt_entry * entry = &idt_entries[vector];
    entry->offset_low = (uint16_t) (isr_addr >> 0);
    entry->selector = selector;
    entry->ist = 0;
    entry->type = type;
    entry->offset_mid = (uint16_t) (isr_addr >> 16);
    entry->offset_high = (uint32_t) (isr_addr >> 32);
    entry->reserved = 0;
}