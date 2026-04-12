#include <stdint.h>
#include "x86_64/util.h"

void pit_init(void)
{
    // Generate interrupt every 1 second;
    uint16_t divisor = 65535 ;
    out_port_B(0x43, 0x36);
    out_port_B(0x40, divisor & 0xFF);     
    out_port_B(0x40, (divisor >> 8));  
}