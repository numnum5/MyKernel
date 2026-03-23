#include "x86_64/util.h"
#include "x86_64/interrupts.h"
#include "x86_64/thread.h"
#include "x86_64/queue.h"
#include "print.h"

queue * threads;
Thread * current_thread;

extern void scheduler_yield(void);

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

void* memcpy(void* dest, const void* src, size_t n) {
    // Cast to char* so we can perform pointer arithmetic byte-by-byte
    char* d = (char*)dest;
    const char* s = (const char*)src;

    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }

    return dest;
}

void switch_context(State_t * state)
{
    pit_ticks++;

    if (pit_ticks >= 18) {
        pit_ticks = 0;
        print_str("interrupt called\n");
        print_uint64_hex(state->frame.rip);
        print_char('\n');


        if (current_thread != NULL)
        {
            print_str("thread available\n");
            print_uint64_hex(current_thread->pid);
            print_char('\n');

            memcpy(&(current_thread->state), state, sizeof(State_t));
            Thread * next_thread = dequeue(threads);

            if (next_thread != NULL){
                print_str("next thread available\n");
                print_uint64_dec(next_thread->pid);
                print_char('\n');
                
                
                enqueue(threads, current_thread);
                
                memcpy(state, &(next_thread->state), sizeof(State_t));

                current_thread = next_thread;
            }

            // memcpy(state, state, sizeof(State_t));
        }
    }

    out_port_B(0x20, 0x20);
    // print_str("\n context switch\n");

    // current_thread->stack_pointer = stack_ptr;
    // Thread * next_thread = dequeue(threads);

    // if (next_thread == NULL)
    // {
    //     return stack_ptr;
    // }

    // enqueue(threads, current_thread);
    
    // current_thread = next_thread;

    // return current_thread->stack_pointer;

    
}

void timer_interrupt_handler() {
    pit_ticks++;
    if (pit_ticks >= 18) {
        pit_ticks = 0;
        print_str("interrupt called\n");
    }

    out_port_B(0x20, 0x20);
	// pic_eoi_master();
}



void thread_wrapper(void (*entry)(void))
{
    print_str("handler\n");
}

void create_thread(void (*entry)(void), uint64_t stack_size)
{
    static uint64_t pids = 5555;
    Thread * thread = pvPortMalloc(sizeof(Thread));
    uint64_t * stack = pvPortMalloc(stack_size);
	thread->pid = pids++;
	thread->state.rdi = (uint64_t) stack;
	thread->state.frame.rip = (uint64_t) (thread_wrapper);
	thread->state.frame.rflags = (1<<1) | (1<<9);
	thread->state.frame.cs = 0x08;
	thread->state.frame.ss = 0x10;
	thread->state.frame.rsp = (uint64_t) stack + stack_size;
    enqueue(threads, thread);
}

void scheduler_init(void)
{
    threads = createQueue();
    Thread * main_thread = pvPortMalloc(sizeof(Thread));
    main_thread->pid = 0xDEADBEEF;
    main_thread->priority = 1;
    enqueue(threads, main_thread);
}


void start_scheduler(void)
{
    Thread * thread = dequeue(threads);

    if (thread != NULL)
    {
        current_thread = thread;
    }
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

    set_idt_gate(32, (uint64_t) scheduler_yield, 0x08, 0x8E);
    
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