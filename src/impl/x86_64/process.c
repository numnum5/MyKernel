
#include <stdint.h>
#include "x86_64/memory.h"

extern void scheduler_yield(uint64_t ptr);

typedef enum 
{
    RUNNING,
    STARTING,
    SLEEPING,
    STOPPED
} State;

typedef struct 
{
    uint64_t pid;
    uint64_t * stack_pointer;
    uint8_t priority;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
    State state;
} Process;

typedef struct 
{
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
} Stackframe __attribute__((packed));


Process current_thread;

void create_thread(void (*entry)(void * arg), uint64_t stack_size)
{
    Process * process = pvPortMalloc(sizeof(Process));
    uint64_t * stack = pvPortMalloc(stack_size);

    // stack points to bottom currently, so we move it up to

    // making them ready for intial start
    process->pid = 1;
    process->priority = 1;
    process->stack_pointer = stack;
    process->state = STARTING;

    
}


void scheduler_init()
{
    // uint64_t * stack = pvPortMalloc(1000);
    // current_thread.pid = 1;
    // current_thread.priority = 1;
    // current_thread.stack_pointer = stack;
}

__attribute__((noreturn)) void start_first_task(uint64_t stack_ptr) {
    asm volatile (
        "mov %0, %%rsp \n"
        "iretq"
        :
        : "r"(stack_ptr)
        : "memory"
    );
}