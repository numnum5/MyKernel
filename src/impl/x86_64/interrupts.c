#include "x86_64/util.h"
#include "x86_64/interrupts.h"
#include "x86_64/thread.h"
#include "x86_64/queue.h"
#include "x86_64/thread.h"
// #include <string.h>
#include "print.h"
#define MLFQ_NLEVELS          5
#define MLFQ_RESET_PERIOD     10000000       
#define MLFQ_LEVEL_RUNTIME(x) ((x) + 1) * 2 

queue * threads;
queue * sleeping_threads;
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

void print_thread(Thread * thread){
    printf("Pid: %x\n", thread->pid);
    printf("rip: %x\n", thread->state.frame.rip);
    printf("cs: %x\n", thread->state.frame.cs);
    printf("cs: %x\n", thread->state.frame.ss);
}

volatile static uint64_t systick = 0;


Thread * schedule()
{
    // Traverse the queue and print each element
    Thread * thread = dequeue(threads);
    while (thread != NULL & thread->status != SLEEPING)
    {
        enqueue(threads, thread);
        thread = dequeue(threads);
    }
}



typedef struct {
    queue queues[MLFQ_NLEVELS];
} MLFQ;

static MLFQ mlfq;

void mlfq_update_level(Thread * thread);

void switch_context(State_t * state)
{
    out_port_B(0x20, 0x20); 
    pit_ticks++;

    if (pit_ticks >= 18) 
    {
        pit_ticks = 0;

        // print_thread(current_thread);

        memcpy(&(current_thread->state), state, sizeof(State_t));
    
        mlfq_update_level(current_thread);

        Thread * nextThread = NULL;

        for(uint8_t i = 0; i < MLFQ_NLEVELS; i++)
        {
            queue * current_queue = &mlfq.queues[i];
            Thread * t = dequeue(current_queue);

            if (t != NULL)
            {
                nextThread = t;
                break;
            }
        }

        enqueue(&mlfq.queues[current_thread->runtime_level], current_thread);

        Thread * next_thread  = dequeue(&mlfq.queues[0]);

        memcpy(state, &(next_thread->state), sizeof(State_t));
        
        current_thread = next_thread;

        systick++;
    }
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
    entry();
    // while(1);
    
}



void start_first_task(CPU_Frame *sp)
{
    asm volatile (
        "mov %0, %%rsp\n\t"
        "iretq\n\t"      
        : 
        : "r"(sp)
        : "memory"
    );
}

Thread * create_thread(void (*entry)(void), uint64_t stack_size, uint64_t pid)
{
    // static uint64_t pids = 5555;
    Thread * thread = pvPortMalloc(sizeof(Thread));
    uint8_t * stack = pvPortMalloc(stack_size);
    thread->pid = pid;
    uint64_t *stack_top = (uint64_t *)(stack + stack_size);
    thread->state.frame.rip = (uint64_t) entry;
    thread->state.frame.rsp = (uint64_t) stack_top;
    thread->state.frame.rflags = 0x202; 
    thread->state.frame.cs = 0x08;
    thread->state.frame.ss = 0x10;


    thread->runtime_level = 0;

    thread->runtime = 0;

    // If it's level 0 then run time duration is only 100ms
    // 1 --> 200ms
    // 2 --> 300ms ...
    thread->runtime_duration = MLFQ_LEVEL_RUNTIME(thread->runtime_level);

    enqueue(&mlfq.queues[0], thread);
    return thread;
    // enqueue(threads, thread);
}



void process_create(void (*entry)(void), uint64_t stack_size, uint64_t pid)
{
    // static uint64_t pids = 5555;
    Thread * thread = pvPortMalloc(sizeof(Thread));
    uint8_t * stack = pvPortMalloc(stack_size);
    
    thread->pid = pid;
    
    uint64_t *stack_top = (uint64_t *)(stack + stack_size);
    
    thread->state.frame.rip = (uint64_t) entry;
    
    thread->state.frame.rsp = (uint64_t) stack_top;
    
    thread->state.frame.rflags = 0x202; 
    
    thread->state.frame.cs = 0x08;
    
    thread->state.frame.ss = 0x10;

    thread->runtime_level = 0;

    thread->runtime = 0;

    // If it's level 0 then run time duration is only 100ms
    // 1 --> 200ms
    // 2 --> 300ms ...
    thread->runtime_duration = MLFQ_LEVEL_RUNTIME(thread->runtime_level);

    enqueue(&mlfq.queues[0], thread);
}


void mlfq_update_level(Thread * thread)
{
    // run time is essentially the same as systick
    thread->runtime++;

    if (thread->runtime == thread->runtime_duration)
    {
        thread->runtime = 0;

        if (thread->runtime_level < MLFQ_NLEVELS - 1)
        {
            thread->runtime_level++;
        }
        
        thread->runtime_duration = MLFQ_LEVEL_RUNTIME(thread->runtime_level);

        printf("Id %x, Runtime: %d, duration: %d, level: %d\n", 
            thread->pid, thread->runtime, thread->runtime_duration, thread->runtime_level);
    }
}


void scheduler_init(void)
{
    threads = createQueue();
    for (uint8_t i = 0; i < MLFQ_NLEVELS; i++)
    {
        mlfq.queues[i].front = NULL;
        mlfq.queues[i].rear = NULL;
    }
    // sleeping_threads = createQueue();
    // Thread * main_thread = pvPortMalloc(sizeof(Thread));
    // main_thread->pid = 0xDEADBEEF;
    // main_thread->priority = 1;
    // enqueue(threads, main_thread);
}


void sleep(uint64_t ms)
{
    // current_thread->sleep_until = systick + ms;
    current_thread->status = SLEEPING;
    scheduler_yield();
}

void start_scheduler(void)
{
    Thread * thread = dequeue(&mlfq.queues[0]);
    if (thread != NULL)
    {
        current_thread = thread;
        current_thread->status = RUNNING;
        start_first_task(&(current_thread->state.frame));
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