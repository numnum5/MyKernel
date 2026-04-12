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
    '*',0,' ' 
};

static const char keymap_shift[128] = {
    0,27,'!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,'A','S','D','F','G','H','J','K','L',':','"','~',
    0,'|','Z','X','C','V','B','N','M','<','>','?',0,
    '*',0,' ' 
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
    while (tty_head == tty_tail); 

    char c = tty_buf[tty_tail];

    tty_tail = (tty_tail + 1) % 256;
    return c;
}
int tty_pos = 0;
bool tty_currently_reading = false;

void tty_readline(char * buf, uint64_t size)
{
    tty_currently_reading = true;

    while (tty_currently_reading)
        asm volatile("hlt");   

    uint64_t i = 0;
    while (tty_buf[i] && i < size - 1) {
        buf[i] = tty_buf[i];
        i++;
    }
    buf[i] = 0;
}

void tty_handle_char(char c)
{
    // vga_putc(c);
    if (c == '\n') {
        tty_buf[tty_pos] = 0;
        tty_pos = 0;
        tty_currently_reading = false;
        return;
    }

    if (c == '\b') {
        if (tty_pos > 0) {
            tty_pos--;
            print_char('\b');
            print_char(' ');
            print_char('\b');
        }
        return;
    }

    if (tty_pos < sizeof(tty_buf) - 1) {
        tty_buf[tty_pos++] = c;
        print_char(c);
    }
}

extern bool tty_printing;
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
    if (!c) return;


    if (tty_currently_reading)
        tty_handle_char(c);
}



void vga_enable_cursor(void)
{
    out_port_B(0x3D4, 0x0A);
    out_port_B(0x3D5, 0);      

    out_port_B(0x3D4, 0x0B);
    out_port_B(0x3D5, 15);   
}

void idt_init(void)
{
    out_port_B(0x20, 0x11); 
    out_port_B(0xA0, 0x11); 

    out_port_B(0x21, 0x20); 
    out_port_B(0xA1, 0x28);

    out_port_B(0x21,0x04);  
    out_port_B(0xA1,0x02);  

    out_port_B(0x21, 0x01); 
    out_port_B(0xA1, 0x01);
    out_port_B(0x21, 0xFD);
    out_port_B(0xA1, 0xFF); 

    idt_ptr.limit = (sizeof(struct idt_entry) * 256) - 1;
	idt_ptr.base = (uint64_t) &idt_entries;

    extern void* isr_table[32];

    for (uint16_t i = 0; i < 32; i++) 
    {
        set_idt_gate(i, isr_table[i], 0x08, 0x8E);
    }

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