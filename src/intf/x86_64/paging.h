#include <stddef.h>
#include <stdint.h>

#define PTE_P  (1ULL << 0)
#define PTE_W  (1ULL << 1)  
#define PTE_U  (1ULL << 2)  
#define PAGE_SIZE 4096
#define MULTIBOOT_MEMORY_AVAILABLE 1
#define MAX_FRAMES (MAX_PHYS_MEM / PAGE_SIZE)
#define configTOTAL_HEAP_SIZE (10 * 1024 * 1024)
#define VIRT_BASE 0xffffffff80000000
#define PHYS_TO_VIRT(x) ((x) + VIRT_BASE)
#define PAGE_PRESENT  (1ULL << 0)
#define PAGE_HUGE     (1ULL << 7)
#define ADDRESS_MASK  0x000FFFFFFFFFF000ULL

typedef struct 
{
    uint64_t present         : 1;   // [0] Page is in RAM
    uint64_t writable        : 1;   // [1] Read/Write (1) or Read-only (0)
    uint64_t user            : 1;   // [2] User-mode (1) or Supervisor-only (0)
    uint64_t write_through   : 1;   // [3] Write-through caching
    uint64_t cache_disable   : 1;   // [4] Disable caching for this page
    uint64_t accessed        : 1;   // [5] Set by CPU when read/written
    uint64_t dirty           : 1;   // [6] Set by CPU when written to
    uint64_t huge_page       : 1;   // [7] PS bit (Must be 0 in PML4 and PT)
    uint64_t global          : 1;   // [8] Prevent TLB flush on CR3 write
    uint64_t available1      : 3;   // [9-11] Free for OS use
    uint64_t physical_addr   : 40;  // [12-51] Physical frame address >> 12
    uint64_t available2      : 11;  // [52-62] Free for OS use
    uint64_t no_execute      : 1;   // [63] Execute Disable (requires EFER.NXE)
} __attribute__((packed)) pt_entry_t;

uint64_t * vmm_translate(uint64_t virt);
void map_page(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags);