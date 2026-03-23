#pragma once
#include <stdint.h>

void vPortFree( void * pv );
void * pvPortMalloc( uint64_t xWantedSize );