#pragma once

#include "x86_64/thread.h"

typedef struct Process {

    uint64_t * stackFrame;

} Process_t;

void idt_initv2(void);
void scheduler_init(void);
void start_scheduler(void);
Thread * create_userthread(void (*entry)(void), uint64_t stack_size, uint64_t pid);
Thread * create_thread(void (*entry)(void), uint64_t stack_size, uint64_t pid);
Process_t* create_process(void (*entry)());
void start_first_task(uint64_t *sp);