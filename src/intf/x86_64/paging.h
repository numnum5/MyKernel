#include <stddef.h>
void pmm_init(void);
uint64_t pmm_alloc_frame(void);
void map_page(uint64_t virt, uint64_t phys, uint32_t flags);
void unmap_identity_mappings();
void *memset(void *dest, int val, size_t len);