#include "x86_64/thread.h"

#define MLFQ_NLEVELS          5
#define MLFQ_RESET_PERIOD     10000000       
#define MLFQ_LEVEL_RUNTIME(x) ((x) + 1) * 2 
typedef struct 
{
    queue queues[MLFQ_NLEVELS];
} MLFQ;

void scheduler_init(void);
void start_scheduler(void);
Thread * create_userthread(void (*entry)(void), uint64_t stack_size, uint64_t pid);
Thread * create_thread(void (*entry)(void), uint64_t stack_size, uint64_t pid);
void start_first_task(uint64_t *sp);