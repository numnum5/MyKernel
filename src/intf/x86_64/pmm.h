
#include <stdint.h>

#define BITMAP_INDEX(x) (x / 64)
#define BITMAP_OFFSET(x) (x % 64)
#define PAGE_SIZE 4096
#define MULTIBOOT_MEMORY_AVAILABLE 1
#define MAX_FRAMES (MAX_PHYS_MEM / PAGE_SIZE)
#define configTOTAL_HEAP_SIZE (10 * 1024 * 1024)
#define VIRT_BASE 0xffffffff80000000
#define PHYS_TO_VIRT(x) ((x) + VIRT_BASE)
#define MULTIBOOT_MEMORY_AVAILABLE              1
#define MULTIBOOT_MEMORY_RESERVED               2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE       3
#define MULTIBOOT_MEMORY_NVS                    4
#define MULTIBOOT_MEMORY_BADRAM                 5


typedef struct 
{
    uint32_t type;
    uint32_t size;
} multiboot_tag;

typedef struct  
{
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t zero;
} multiboot_mmap_entry;

typedef struct 
{
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    multiboot_mmap_entry entries[];
} multiboot_tag_mmap;

void pmm_init(uint64_t multibootinfo);
uint64_t * pmm_alloc_frame(void);
