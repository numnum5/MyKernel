#include <stdint.h>
#include <stddef.h>

typedef struct MemoryRegion {
    uint64_t size;
    uint64_t addr;
    uint32_t type;
    struct MemoryRegion* next;
} MemoryRegion;

void printNodes(MemoryRegion * head);
void push_back(MemoryRegion ** head, struct MemoryRegion * element);