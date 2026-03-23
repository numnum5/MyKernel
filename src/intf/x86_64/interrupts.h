#pragma once

void idt_initv2(void);
void scheduler_init(void);
void start_scheduler(void);
void create_thread(void (*entry)(void), uint64_t stack_size);