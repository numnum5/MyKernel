
#include <stdint.h>
#include "x86_64/thread.h"
#include "print.h"

typedef enum {
    SYS_YIELD,
    SEND,
    SYS_WRITE,
    SYS_EXIT

} Syscall;


uint64_t syscall_handler(State_t* state)
{

    printf("SYSCALLED\n");

    switch (state->rax) {
        case SYS_WRITE:
            return 0;

        case SYS_YIELD:
            // schedule_yield(state);   // voluntary switch
            return 0;

        case SYS_EXIT:
            // thread_exit();
            // schedule_yield(state);   // switch away permanently
            return 0;

        default:
            return -1;
    }
}
