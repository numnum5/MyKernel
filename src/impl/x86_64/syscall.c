
#include <stdint.h>
#include "x86_64/thread.h"
#include "print.h"

typedef enum {
    SYS_WRITE,
    SYS_YIELD,
    SYS_EXIT

} Syscall;

typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rsi, rdi, rbp, rdx, rcx, rbx, rax;
} __attribute__((packed)) syscall_reg_t;


void syscall_handler(syscall_reg_t* state)
{



    printf("SYSCALLED\n");


    printf("rax: %x\n", state->rax);
    printf("rdi: %x\n", state->rdi);
    printf("rbx: %x\n", state->rbx);
    printf("rsi: %x\n", state->rsi);

    char * string = (char*) state->rdi;

    printf(string);
   


    // printf("%x\n", state->rax);
    while(1);
    // return 0;
    // switch (state->rax) {
    //     case SYS_WRITE:
    //         return 0;

    //     case SYS_YIELD:
    //         // schedule_yield(state);   // voluntary switch
    //         return 0;

    //     case SYS_EXIT:
    //         // thread_exit();
    //         // schedule_yield(state);   // switch away permanently
    //         return 0;

    //     default:
    //         return -1;
    // }
}
