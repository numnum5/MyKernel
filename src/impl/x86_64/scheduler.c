#include "x86_64/util.h"
#include "x86_64/thread.h"
#include "x86_64/scheduler.h"
#include "x86_64/stdlib.h"
#include "print.h"

queue * threads;
queue * sleeping_threads;
Thread * current_thread;

static MLFQ mlfq;
static volatile uint32_t pit_ticks = 0;
static volatile uint64_t systick = 0;
extern cpu_local_data single_cpu;
extern void scheduler_yield(void);

void print_thread(Thread * thread){
    printf("Pid: %x\n", thread->pid);
    printf("rip: %x\n", thread->state.frame.rip);
    printf("cs: %x\n", thread->state.frame.cs);
    printf("cs: %x\n", thread->state.frame.ss);
}

Thread * schedule()
{
    Thread * thread = dequeue(threads);
    while (thread != NULL & thread->status != SLEEPING)
    {
        enqueue(threads, thread);
        thread = dequeue(threads);
    }
}

void mlfq_update_level(Thread * thread);

void switch_context(State_t * state)
{
    out_port_B(0x20, 0x20); 
    pit_ticks++;

    if (pit_ticks >= 18) 
    {
        pit_ticks = 0;

        print_thread(current_thread);
        printf("Current thread rsp %x\n", state->frame.rsp);
        memcpy(&(current_thread->state), state, sizeof(State_t));
        
    
        if (current_thread->thread_type == KERNEL)
        {
            single_cpu.kernel_stack = state->frame.rsp;
            printf("kernel stack: %x\n", single_cpu.kernel_stack);
        }
        else if (current_thread->thread_type == USER)
        {
            single_cpu.user_stack = state->frame.rsp;
            printf("user stack: %x\n", single_cpu.user_stack);
        }

        
        printf("pit tick%d\n", pit_ticks);

        
        // mlfq_update_level(current_thread);

        // Thread * nextThread = NULL;

        // for(uint8_t i = 0; i < MLFQ_NLEVELS; i++)
        // {
        //     queue * current_queue = &mlfq.queues[i];
        //     Thread * t = dequeue(current_queue);

        //     if (t != NULL)
        //     {
        //         nextThread = t;
        //         break;
        //     }
        // }

        enqueue(&mlfq.queues[0], current_thread);

        Thread * next_thread  = dequeue(&mlfq.queues[0]);

        printf("info abotu next thread:\n");
        print_thread(next_thread);
        
        memcpy(state, &(next_thread->state), sizeof(State_t));

        printf("state rip: %x\n", state->frame.rip);
        // printf("state: %x\n", );
        // state = &(next_thread->state);
        
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
    // print_str("handler\n");
    entry();
    // while(1);
    
}


void create_thread_ring(Thread * thread)
{

    printf("thread pid: %x\n", thread->pid);
    enqueue(&mlfq.queues[0], thread);
}

void create_thread(void (*entry)(void), uint64_t stack_size, uint64_t pid)
{
    Thread * thread = malloc(sizeof(Thread));
    uint8_t * stack = malloc(stack_size);
    thread->pid = pid;
    thread->thread_type = KERNEL;
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



void process_create(void (*entry)(void), uint64_t stack_size, uint64_t pid)
{
    // static uint64_t pids = 5555;
    Thread * thread = malloc(sizeof(Thread));
    uint8_t * stack = malloc(stack_size);
    
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


void start_first_task(uint64_t *frame)
{
    asm volatile (
        "mov %0, %%rsp;"  
        "iretq;"
        :
        : "r"(frame)
        : "memory"
    );
}

void start_scheduler(void)
{
    Thread * thread = dequeue(&mlfq.queues[0]);
    if (thread != NULL)
    {
        current_thread = thread;
        current_thread->status = RUNNING;

        printf("frame: %x\n", &(current_thread->state.frame));
        printf("rsp: %x\n",current_thread->state.frame.rsp);


        start_first_task(&(current_thread->state.frame));
    }
}