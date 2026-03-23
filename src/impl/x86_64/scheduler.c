
#include <stdint.h>
#include <stddef.h>
#include "x86_64/memory.h"
#include "x86_64/queue.h"
#include "x86_64/scheduler.h"
#include "print.h"


// queue * threads = NULL;
// Thread * current_thread = NULL;

// void create_thread(void (*entry)(void * arg), uint64_t stack_size)
// {
//     Thread * process = pvPortMalloc(sizeof(Thread));
//     uint64_t * stack = pvPortMalloc(stack_size);

//     // stack points to bottom currently, so we move it up to

//     // making them ready for intial start
//     process->pid = 1;
//     process->priority = 1;
//     process->stack_pointer = stack;
//     process->state = STARTING;


//     enqueue(threads, process);

// }

// void scheduler_init()
// {
//     threads = createQueue();
//     // uint64_t * stack = pvPortMalloc(1000);
//     // current_thread.pid = 1;
//     // current_thread.priority = 1;
//     // current_thread.stack_pointer = stack;
// }


// void start_scheduler()
// {
//     Thread * thread = dequeue(threads);

//     if (thread != NULL)
//     {
//         current_thread = thread;
//     }
// }