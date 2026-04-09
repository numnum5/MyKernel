#include "x86_64/idt.h"
#include <stddef.h>

#include "print.h"
#include "bool.h"
#include "x86_64/ps2.h"
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

extern void keyboard_handler_wrapped();


#define TTY_SIZE 256

static const char keymap[128] = {
    0,27,'1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',0,
    '*',0,' '   // <-- added entries up to 0x39
};

static const char keymap_shift[128] = {
    0,27,'!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,'A','S','D','F','G','H','J','K','L',':','"','~',
    0,'|','Z','X','C','V','B','N','M','<','>','?',0,
    '*',0,' '   // <-- added entries up to 0x39
};
char tty_buf[TTY_SIZE];
int tty_head = 0;
int tty_tail = 0;
static int shift = 0;



int startswith(const char* str, const char* prefix)
{
    while (*prefix) {
        if (*str++ != *prefix++)
            return 0;
    }
    return 1;
}

char tty_read(void)
{
    while (tty_head == tty_tail); // wait for input

    char c = tty_buf[tty_tail];

    tty_tail = (tty_tail + 1) % 256;
    return c;
}
void read_line(char* buf, uint64_t max)
{
    uint64_t i = 0;

    // 10 == 9 
    while (i < max - 1)
    {
        char c = tty_read();
        if (c == '\n')
        {
            vga_putc('\n');
            break;
        }

        if (c == '\b')
        {
            if (i > 0)
            {
                i--;
                vga_putc('\b');
                vga_putc(' ');
                vga_putc('\b');
            }
            continue;
        }

        buf[i++] = c;
        vga_putc(c); // echo
    }
    
    buf[i] = 0;
}

void keyboard_handler()
{
    out_port_B(0x20, 0x20); 
    uint8_t sc = ps2_read_scan_code();

    if (sc & 0x80)
        return;

    if (sc == 42 || sc == 54) 
    {
        shift = 1;
        return;
    }

    char c = shift ? keymap_shift[sc] : keymap[sc];
    if (!c)
        return;

    int next = (tty_head + 1) % TTY_SIZE;
   
    if (next != tty_tail)
    {
        tty_buf[tty_head] = c;
        tty_head = next;
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
    // out_port_B(0x21, 0xFE); // 11111110 → enable IRQ0 only
    // 1111 1100
    //      8 4 2 0 A B D E 
    out_port_B(0x21, 0xFD);
    out_port_B(0xA1, 0xFF); // keep slave masked for now

    idt_ptr.limit = (sizeof(struct idt_entry) * 256) - 1;
	idt_ptr.base = (uint64_t) &idt_entries;

// Before setting specific gates:
    extern void* isr_table[32];

    for (uint16_t i = 0; i < 32; i++) 
    {
        set_idt_gate(i, isr_table[i], 0x08, 0x8E);
    }

    // set_idt_gate(32, (uint64_t) scheduler_yield, 0x08, 0x8E);
    set_idt_gate(0x21, (uint64_t) keyboard_handler_wrapped, 0x08, 0x8E);

    for (uint16_t i = 34; i < 256; i++) 
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