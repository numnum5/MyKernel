#include "x86_64/list.h"
#include "x86_64/pmm.h"

static MemoryRegion * memoryRegions = NULL;
static uint64_t * bitmap = NULL;
static uint64_t total_memory_size = 0;
static uint64_t total_pages = 0;
static uint64_t total_frames;
static uint64_t bitmap_size_bytes;

extern uint64_t heap_start;

static void set_frame(uint64_t frame) 
{
    bitmap[BITMAP_INDEX(frame)] |= (1ULL << BITMAP_OFFSET(frame));
}

static void clear_frame(uint64_t frame) 
{
    bitmap[BITMAP_INDEX(frame)] &= ~(1ULL << BITMAP_OFFSET(frame));
}

static int test_frame(uint64_t frame) 
{
    return bitmap[BITMAP_INDEX(frame)] & (1ULL << BITMAP_OFFSET(frame));
}


void parse_mmap(multiboot_tag_mmap * mmap_tag) 
{
    uint32_t num_entries = (mmap_tag->size - sizeof(multiboot_tag_mmap)) / mmap_tag->entry_size;

    for (uint32_t i = 0; i < num_entries; i++)
    {
        multiboot_mmap_entry * entry = &mmap_tag->entries[i];

        MemoryRegion * region = pvPortMalloc(sizeof(MemoryRegion));
        region->addr = entry->addr;
        region->size = entry->len;
        region->type = entry->type;
        printf("addr: %x size: %x\n", region->addr, region->size);

        push_back(&memoryRegions, region);
    }
}

void pmm_init(uint64_t multibootinfo)
{
    uint64_t multibootinfo_virt = (uint64_t)multibootinfo + VIRT_BASE;

    multiboot_tag * tag;
    for (tag = (multiboot_tag *) (multibootinfo_virt + 8); 
    tag->type != 0; 
    tag = (multiboot_tag *) ((uint8_t *) tag + ((tag->size + 7) & ~7))) 
    {
        if (tag->type == 6) {
            multiboot_tag_mmap * mmap = (multiboot_tag_mmap *) tag;
            parse_mmap(mmap);
        }
    }

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

    // Calculate bitmap size (ceil it up to nearest byte)
    bitmap_size_bytes = (total_pages + 7) / 8;

    // align with 4096 kb size so that when we mark the memory as used it doesn't reallocate excess bits
    bitmap_size_bytes = (bitmap_size_bytes + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    uint64_t bitmap_addr = (uint64_t) pvPortMalloc(bitmap_size_bytes);
	bitmap = (uint64_t *) bitmap_addr;

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

	for (uint64_t f = first_reserved; f < last_reserved; f++)
	{
		set_frame(f);
	}
}

uint64_t * pmm_alloc_frame(void)
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
                {
                    break;
                }
                    
                if (!test_frame(frame))
                {
                    set_frame(frame); 
                    return (uint64_t *) (frame * PAGE_SIZE);
                }
            }
        }
    }

    return NULL;  // out of memory
}