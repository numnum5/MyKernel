#include <stdint.h>

void heap_init(uint64_t heap_start, uint64_t heap_size);
void add_memory_region(uint64_t start_addr, uint64_t size);
struct Node * find_memory(uint64_t size);
uint64_t * kmalloc(uint64_t size);