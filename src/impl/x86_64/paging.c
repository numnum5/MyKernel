#include <stdint.h>
#include <stddef.h>
#include "x86_64/paging.h"
#include "x86_64/list.h"
#include "x86_64/memory.h"
#include "print.h"

#define PAGE_SIZE 4096
#define MULTIBOOT_MEMORY_AVAILABLE 1
#define MAX_FRAMES (MAX_PHYS_MEM / PAGE_SIZE)
#define configTOTAL_HEAP_SIZE (10 * 1024 * 1024)
#define VIRT_BASE 0xffffffff80000000
#define PHYS_TO_VIRT(x) ((x) + VIRT_BASE)

extern MemoryRegion * memoryRegions;

typedef struct {
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
} __attribute__((packed)) pt_entry_t;;


static uint64_t * bitmap;
uint64_t get_l4_page_table2(void)
{
    uint64_t cr3;
    asm volatile ("mov %%cr3, %0" : "=r"(cr3));
    return cr3 & 0x000FFFFFFFFFF000ULL;
}

uint64_t *get_pml4_virt(void)
{
    uint64_t phys = get_l4_page_table2();
    return (uint64_t*)PHYS_TO_VIRT(phys);
}

#define BITMAP_INDEX(x) (x / 64)
#define BITMAP_OFFSET(x) (x % 64)

static void set_frame(uint64_t frame) {
    bitmap[BITMAP_INDEX(frame)] |= (1ULL << BITMAP_OFFSET(frame));
}

static void clear_frame(uint64_t frame) {
    bitmap[BITMAP_INDEX(frame)] &= ~(1ULL << BITMAP_OFFSET(frame));
}

static int test_frame(uint64_t frame) {
    return bitmap[BITMAP_INDEX(frame)] & (1ULL << BITMAP_OFFSET(frame));
}

static uint64_t total_memory_size = 0;
static uint64_t total_pages = 0;
extern uint64_t heap_start;
static uint64_t total_frames;
static uint64_t bitmap_size_bytes;

void pmm_init(void)
{
	// uint64_t number_regions = 0;
	MemoryRegion * current = memoryRegions;

	while (current != NULL)
	{
		// number_regions++;
        if (current->addr != 0)
        {
            total_memory_size += current->size;
        }
		current = current->next;
	}

	total_pages = total_memory_size / PAGE_SIZE;
	total_frames = total_pages;

	// print_clear();//++
    printf("total memory %d\n", total_memory_size);
	print_str("\ntotal_pages: \n");
	print_uint64_dec(total_pages);


    // Calculate bitmap size (ceil it up to nearest byte)
    bitmap_size_bytes = (total_pages + 7) / 8;

    // align with 4096 kb size so that when we mark the memory as used it doesn't reallocate excess bits
    bitmap_size_bytes = (bitmap_size_bytes + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

	// print_str("\n bitmap_size_bytes: \n");
	// print_uint64_dec(bitmap_size_bytes);
	// print_char('\n');

    uint64_t bitmap_addr = (uint64_t) pvPortMalloc(bitmap_size_bytes);

	bitmap = (uint64_t *) bitmap_addr;
	// print_str("\n bitmap_size_bytes: \n");
	// print_uint64_dec((uint64_t) bitmap_size_bytes / 8);
	// print_char('\n');
 
    for (uint64_t i = 0; i < bitmap_size_bytes / 8; i++)
	{
        bitmap[i] = 0xFFFFFFFFFFFFFFFFULL;
	}

	current = memoryRegions;

	while (current != NULL)
	{
		if (current->type == MULTIBOOT_MEMORY_AVAILABLE)
		{


			uint64_t start = current->addr;
			uint64_t end = current->addr + current->size;
			uint64_t first_frame = start / PAGE_SIZE;
			uint64_t last_frame  = end   / PAGE_SIZE;

            print_str("\n start: \n");;
            print_uint64_hex(start);
            print_char('\n');
            print_uint64_hex(end);
            print_char('\n');

            if (current->addr != 0)
            {
                for (uint64_t f = first_frame; f < last_frame; f++)
                {
                    clear_frame(f);
                }	                
            }
            else
            {
                for (uint64_t f = first_frame; f < last_frame; f++)
                {
                    set_frame(f);
                }	
            }
		
		}
		current = current->next;
	}

	uint64_t kernel_heap_start = heap_start - VIRT_BASE;
	uint64_t kernel_heap_end = kernel_heap_start + configTOTAL_HEAP_SIZE;
	uint64_t first_reserved = kernel_heap_start / PAGE_SIZE;
    uint64_t last_reserved  = (kernel_heap_end + PAGE_SIZE - 1) / PAGE_SIZE;

    // print_str("\n kernel_heap_start: \n");;
    // print_uint64_dec(first_reserved);
    // print_char('\n');
    // print_uint64_dec(last_reserved);
    // print_char('\n');

	for (uint64_t f = first_reserved; f < last_reserved; f++)
	{
		set_frame(f);
	}
    // print_str("\n kernel_heap_start: \n");
}

uint64_t pmm_alloc_frame(void)
{
    uint64_t bitmap_entries = (total_frames + 63) / 64;

    for (uint64_t i = 0; i < bitmap_entries; i++)
    {
        if (bitmap[i] != 0xFFFFFFFFFFFFFFFFULL)
        {
            for (uint64_t bit = 0; bit < 64; bit++)
            {
                uint64_t frame = i * 64 + bit;

                if (frame >= total_frames)
                    break;

                if (!test_frame(frame))
                {
                    set_frame(frame); 
                    return frame * PAGE_SIZE;
                }
            }
        }
    }

    return 0;  // out of memory
}
#define PTE_P  (1ULL << 0)  // 0x001
#define PTE_W  (1ULL << 1)  // 0x002
#define PTE_U  (1ULL << 2)  // 0x004
// // Simplified mapping function

void *memset(void *dest, int val, size_t len)
{
    uint8_t *ptr = (uint8_t *)dest;

    for (size_t i = 0; i < len; i++) {
        ptr[i] = (uint8_t)val;
    }

    return dest;
}


void unmap_identity_mappings() {
    uint64_t* pml4 = get_pml4_virt();
    
    // Clear the first entry (covers 0x0 to 0x0000007fffffffff)
    pml4[0] = 0;

    // Flush the TLB so the CPU realizes the mapping is gone
    asm volatile("mov %%cr3, %%rax; mov %%rax, %%cr3" ::: "rax", "memory");
}


extern uint8_t page_table_l4[];



void map_page(uint64_t virt, uint64_t phys, uint32_t flags)
{
    // 1. Extract indices (9 bits each)
    uintptr_t pml4_idx = (virt >> 39) & 0x1FF;
    uintptr_t pdpt_idx = (virt >> 30) & 0x1FF;
    uintptr_t pd_idx   = (virt >> 21) & 0x1FF;

    uint64_t* pml4 = get_pml4_virt();

    printf("from cr3: %x\n", pml4);
    printf("From asm: %x\n", (uint64_t)page_table_l4);

    if (!(pml4[pml4_idx] & PTE_P)) 
    {
        uint64_t frame = pmm_alloc_frame();
        pml4[pml4_idx] = frame | PTE_P | PTE_W | PTE_U; 
    }
    uint64_t* pdpt = (uint64_t*)PHYS_TO_VIRT(pml4[pml4_idx] & 0x000FFFFFFFFFF000ULL);

    if (!(pdpt[pdpt_idx] & PTE_P)) {
        uint64_t frame = pmm_alloc_frame();
        pdpt[pdpt_idx] = frame | PTE_P | PTE_W | PTE_U;
    }

    uint64_t* pd = (uint64_t*)PHYS_TO_VIRT(pdpt[pdpt_idx] & 0x000FFFFFFFFFF000ULL);

    pd[pd_idx] = (phys & 0x000FFFFFFFFFF000ULL) | flags | PTE_P;

    // // The CPU caches translations; we must tell it this address changed.
    asm volatile("invlpg (%0)" :: "r"(virt) : "memory");
}