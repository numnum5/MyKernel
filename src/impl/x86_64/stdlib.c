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

bool memcmp(void * source, void * dest, size_t size)
{
    char *source_ptr = (char *) source;
    char *des_ptr = (char *)dest;

    for (uint64_t i = 0; i < size; i++)
    {
        if (source_ptr[i] != des_ptr[i])
        {
            return false;
        }
    }
    return true;
}

void * malloc(size_t size)
{
    return pvPortMalloc(size);
}


void free(void * ptr)
{
    vPortFree(ptr);
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
