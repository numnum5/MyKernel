#include <stdint.h>
#include <stddef.h>
#include "bool.h"
void * malloc(size_t size);
void * memset(void *dest, int val, size_t len);
void* memcpy(void* dest, const void* src, size_t n);
bool memcmp(void * source, void * dest, size_t size);
void free(void * ptr);