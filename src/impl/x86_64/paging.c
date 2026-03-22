#include <stdint.h>
#include "x86_64/paging.h"
#include "x86_64/list.h"
#include "x86_64/memory.h"
#include "print.h"

#define PAGE_SIZE 4096
#define MULTIBOOT_MEMORY_AVAILABLE 1
// #define MAX_PHYS_MEM (1024ULL * 1024 * 1024)  // 1GB example
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
uint64_t get_l4_page_table(void)
{
    uint64_t cr3;
    asm volatile ("mov %%cr3, %0" : "=r"(cr3));
    return cr3 & 0x000FFFFFFFFFF000ULL;
}

uint64_t *get_pml4_virt(void)
{
    uint64_t phys = get_l4_page_table();
    return (uint64_t*)PHYS_TO_VIRT(phys);
}

#define BITMAP_INDEX(x) (x / 64)
#define BITMAP_OFFSET(x) (x % 64)



// frame = 100;

// BITMAP_INDEX(frame) = 1

// BITMAP_OFFSET(frame) = 26
static void set_frame(uint64_t frame) {
    bitmap[BITMAP_INDEX(frame)] |= (1ULL << BITMAP_OFFSET(frame));
}

static void clear_frame(uint64_t frame) {
    bitmap[BITMAP_INDEX(frame)] &= ~(1ULL << BITMAP_OFFSET(frame));
}

static int test_frame(uint64_t frame) {
    return bitmap[BITMAP_INDEX(frame)] & (1ULL << BITMAP_OFFSET(frame));
}
uint64_t * get_virt();

static uint64_t total_memory_size = 0;
static uint64_t total_pages = 0;

// void pmm_init(uint64_t page_size)
// {
// 	MemoryRegion * current = memoryRegions;
// 	while (current != NULL)
// 	{
// 		total_memory_size += current->size;
// 		current = current->next;
// 	}

// 	total_pages = total_memory_size / PAGE_SIZE;

// 	bitmap = pvPortMalloc(total_pages);


// 	print_clear();
// 	print_str("\ntotal_pages: \n");
// 	print_uint64_hex(total_pages);
// 	print_str("\ntotal_memory_size: \n");
// 	print_uint64_hex(total_memory_size);
// 	print_char('\n');

extern uint64_t heap_start;


// }
static uint64_t total_frames;
static uint64_t bitmap_size_bytes;

void pmm_init(void)
{
	// uint64_t number_regions = 0;
	MemoryRegion * current = memoryRegions;
	while (current != NULL)
	{
		// number_regions++;
		total_memory_size += current->size;
		current = current->next;
	}

	total_pages = total_memory_size / PAGE_SIZE;
	total_frames = total_pages;

	print_clear();//++
	print_str("\ntotal_pages: \n");
	print_uint64_dec(total_pages);


    // 2️⃣ Calculate bitmap size (ceil it up to nearest byte)
    bitmap_size_bytes = (total_pages + 7) / 8;

    // align with 4096 kb size so that when we mark the memory as used it doesn't reallocate excess bits
    bitmap_size_bytes = (bitmap_size_bytes + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

	print_str("\n bitmap_size_bytes: \n");
	print_uint64_dec(bitmap_size_bytes);
	print_char('\n');

    uint64_t bitmap_addr = (uint64_t) pvPortMalloc(bitmap_size_bytes);

	bitmap = (uint64_t *) bitmap_addr;
	print_str("\n bitmap_size_bytes: \n");
	print_uint64_dec((uint64_t) bitmap_size_bytes / 8);
	print_char('\n');
 
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


			for (uint64_t f = first_frame; f < last_frame; f++)
			{
				clear_frame(f);
			}			
		}
		current = current->next;
	}

	uint64_t kernel_heap_start = heap_start - VIRT_BASE;
	uint64_t kernel_heap_end = kernel_heap_start + configTOTAL_HEAP_SIZE;
	uint64_t first_reserved = kernel_heap_start / PAGE_SIZE;
    uint64_t last_reserved  = (kernel_heap_end + PAGE_SIZE - 1) / PAGE_SIZE;

    print_str("\n kernel_heap_start: \n");;
    print_uint64_dec(first_reserved);
    print_char('\n');
    print_uint64_dec(last_reserved);
    print_char('\n');

	for (uint64_t f = first_reserved; f < last_reserved; f++)
	{
		set_frame(f);
	}
    print_str("\n kernel_heap_start: \n");
}

uint64_t * pmm_alloc_frame(void)
{

    uint64_t counter = 0;
    uint64_t bitmap_entries = (total_frames + 63) / 64;
    for (uint64_t i = 0; i < bitmap_entries; i++)
    {
        if (bitmap[i] != 0xFFFFFFFFFFFFFFFFULL)
        {
            for (uint64_t bit = 0; bit < 64; bit++)
            {
                uint64_t frame = i * 64 + bit;
                if (! test_frame(frame))
                {

                    if (i != 0)
                    {
                        return (uint64_t *) (frame * PAGE_SIZE);
                    }
                    counter++;
                }
            }
        }
    }

    // print_str("frame counter\n");
    // print_uint64_dec(counter);
    // print_char('\n');;
    // for (uint64_t i = 0; i < bitmap_entries; i++)
    // {
    //     // If all 64 bits are 1 → fully used → skip
    //     if (bitmap[i] == 0xFFFFFFFFFFFFFFFFULL)
    //     {
    //         continue;
    //     }

    //     // At least one free bit exists
    //     for (uint64_t bit = 0; bit < 64; bit++)
    //     {
    //         uint64_t frame = i * 64 + bit;
    //         if (frame >= total_frames)
    //         {
    //             break;
    //         }

    //         if (! test_frame(frame))
    //         {
    //             return frame * PAGE_SIZE;   // return physical address
    //         }
    //     }
    // }

    return NULL;  // out of memory
}

// // Simplified mapping function
void map_page(uint64_t virt, uint64_t phys, uint32_t flags)
{
    // 1. Extract indices from the virtual address
    // Each index is 9 bits wide
    uint64_t pml4_idx = (virt >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
    uint64_t pd_idx   = (virt >> 21) & 0x1FF;
    uint64_t pt_idx   = (virt >> 12) & 0x1FF;

    // 2. Traverse the levels
    // 'get_virt' converts a physical address to your kernel's access window
    pt_entry_t * pml4 = (pt_entry_t*)get_pml4_virt();
    
    if (! pml4[pml4_idx].present) 
    {
        uint64_t * frame = pmm_alloc_frame();

        if (frame == NULL)
        {
            return;
        }


        pml4[pml4_idx];
        // allocate_table_level(&pml4[pml4_idx]);
    }

    // pt_entry_t* pdpt = (pt_entry_t*)get_virt(pml4[pml4_idx].physical_addr << 12);


    // if (!pdpt[pdpt_idx].present) {
    //     // allocate_table_level(&pdpt[pdpt_idx]);
    // }

	// /// 000000000
    // pt_entry_t* pd = (pt_entry_t*)get_virt(pdpt[pdpt_idx].physical_addr << 12);
    // if (!pd[pd_idx].present) {
    //     // allocate_table_level(&pd[pd_idx]);
    // }

    // pt_entry_t* pt = (pt_entry_t*)get_virt(pd[pd_idx].physical_addr << 12);

    // // 3. Set the actual Page Table Entry
    // pt[pt_idx].physical_addr = phys >> 12;
    // pt[pt_idx].present = 1;
    // pt[pt_idx].writable = (flags & PAGE_WRITABLE) ? 1 : 0;
    
    // 4. TLB Invalidation (CRITICAL)
    // The CPU caches translations; we must tell it this address changed.
    // asm volatile("invlpg (%0)" :: "r"(virt) : "memory");
}