#pragma once
#include <stdint.h>


typedef struct {
    uint64_t unused;       // Offset 0
    uint64_t kernel_stack; // Offset 8
    uint64_t user_stack;   // Offset 16
} __attribute__((packed)) cpu_local_data;

typedef struct {
    uint64_t self;          // gs:0 (very useful)
    uint64_t kernel_stack;  // gs:8
    uint64_t user_stack;    // gs:16
} __attribute__((packed)) cpu_local;

void out_port_B(uint16_t port, uint8_t value);