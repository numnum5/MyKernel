#include <stdint.h>
#define SYSCALL_NUM 10

typedef enum {
    SYS_WRITE,
    SYS_YIELD,
    SYS_EXIT

} Syscall;

typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rsi, rdi, rbp, rdx, rcx, rbx, rax;
} __attribute__((packed)) syscall_reg_t;

typedef void (*syscall_handler_t)(syscall_reg_t *r);


void init_syscalls(void);
void syscall_write(syscall_reg_t *r);
void syscall_default(syscall_reg_t * r);