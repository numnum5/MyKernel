#pragma once
#include <stdint.h>
typedef struct StackFrame {
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
}StackFrame_t __attribute__((packed));

typedef struct State {
    uint64_t rbp;
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    StackFrame_t frame;
} State_t __attribute__((packed));;

typedef enum 
{
    RUNNING,
    STARTING,
    SLEEPING,
    STOPPED
} Status;

typedef struct 
{
    uint64_t pid;
    Status status;
    uint8_t priority;
    State_t state;
} Thread;
