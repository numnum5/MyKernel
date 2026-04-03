#pragma once
#include <stdint.h>
typedef struct {
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed)) CPU_Frame;

typedef struct __attribute__((packed)) {
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
    CPU_Frame frame;
} State_t;

typedef struct __attribute__((packed)) {
    uint64_t ss;
    uint64_t rsp;
    uint64_t rflags;
    uint64_t cs;
    uint64_t rip;
    uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t r11;
	uint64_t r10;
	uint64_t r9;
	uint64_t r8;
	uint64_t rdi;
	uint64_t rsi;
	uint64_t rdx;
	uint64_t rcx;
	uint64_t rbx;
	uint64_t rax;
	uint64_t rbp;
} State_reversed;

typedef enum 
{
    RUNNING,
    STARTING,
    SLEEPING,
    STOPPED
} Status;

typedef enum 
{
    USER,
    KERNEL
} Thread_Type;

typedef struct 
{
    uint8_t runtime_level;

    /** How much the thread has spent using the CPU. */
    uint64_t runtime;
    /** How long the thread can run before get demoted in level. */
    uint64_t runtime_duration;
    uint64_t sleep_until;
    uint64_t pid;
    Status status;
    uint8_t priority;
    State_t state;
    Thread_Type thread_type;
    uint64_t * stack_pointer;
} Thread;
