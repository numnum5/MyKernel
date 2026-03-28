#include "x86_64/memory.h"
#include "x86_64/stdlib.h"

void *memset(void *dest, int val, size_t len)
{
    uint8_t *ptr = (uint8_t *)dest;

    for (size_t i = 0; i < len; i++) {
        ptr[i] = (uint8_t)val;
    }

    return dest;
}


void * malloc(size_t size)
{
    return pvPortMalloc(size);
}


void* memcpy(void* dest, const void* src, size_t n) 
{
    // Cast to char* so we can perform pointer arithmetic byte-by-byte
    char* d = (char*)dest;
    const char* s = (const char*)src;

    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }

    return dest;
}
