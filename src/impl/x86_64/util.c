#include <stdint.h>


void out_port_B(uint16_t port, uint8_t value)
{
    asm volatile("out %0, %1" ::"a"(value), "Nd"(port));
}
