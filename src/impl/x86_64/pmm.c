#include "x86_64/list.h"
#include "x86_64/pmm.h"
#include "x86_64/stdlib.h"
#include "x86_64/paging.h"
#include "print.h"
#define USER_STACK_START (0x7FFFFFFFFFF0 - 0x200000)
static MemoryRegion * memoryRegions = NULL;
static uint64_t * bitmap = NULL;
static uint64_t total_memory_size = 0;
static uint64_t total_pages = 0;
static uint64_t total_frames;
static uint64_t bitmap_size_bytes;
static FileSystem fs;
static uint64_t num_64_bits_needed;
extern uint64_t heap_start;
extern uint8_t __kernel_start[];
extern uint8_t __kernel_end[];

static uint8_t pmm_get_bit(uint64_t page) {
    uint8_t bit = page % 64;
    uint64_t byte = page / 64;

    // printf("%d , %llu, %llx\n", bit, byte, bitmap[byte]);

    return (bitmap[byte] >> bit) & 1ULL;
}


static void clear_frame(uint64_t f) 
{
    uint64_t byte_index = f / 64;
    uint8_t bit_index = f % 64; 

    // msb 000 ... 0001 lsb

    bitmap[byte_index] &= ~(1ULL << bit_index);
}

static void set_frame(uint64_t f) 
{
    uint64_t byte_index = f / 64;
    uint8_t bit_index = f % 64; 

    // msb 000 ... 0001 lsb

    bitmap[byte_index] |= (1ULL << bit_index);
}

void parse_mmap(multiboot_tag_mmap * mmap_tag) 
{
    uint32_t num_entries = (mmap_tag->size - sizeof(multiboot_tag_mmap)) / mmap_tag->entry_size;

    for (uint32_t i = 0; i < num_entries; i++)
    {
        multiboot_mmap_entry * entry = &mmap_tag->entries[i];


        MemoryRegion * region = (MemoryRegion *)malloc(sizeof(MemoryRegion));
        region->addr = entry->addr;
        region->size = entry->len;
        region->type = entry->type;
        printf("addr: %x size: %x\n", region->addr, region->size);

        push_back(&memoryRegions, region);
        
    }
}

File* fs_open(FileSystem *fs, const char *name)
{
    uint32_t cluster = fs->root_cluster;


    printf("root clustera: %d\n", fs->root_cluster);
    // Walk directory cluster chain
    while (cluster < 0x0FFFFFF8)
    {
        uint32_t sector = cluster_to_sector(fs, cluster);

        printf("sector: %d\n", sector);
        printf("sectors per clusters: %d\n", fs->sectors_per_cluster);
        // clusers => [sector1, sector2, sector3, ....]

        // For each sector in this cluster
        for (uint32_t s = 0; s < fs->sectors_per_cluster; s++)
        {
            fat_dir_entry *dir = (fat_dir_entry*) (fs->disk + (sector + s) * fs->bytes_per_sector);

            uint8_t entries_per_sector = fs->bytes_per_sector / sizeof(fat_dir_entry);
            // printf("sector: %d\n", entries_per_sector);

            for (uint8_t i = 0; i < entries_per_sector; i++)
            {
                fat_dir_entry *e = &dir[i];

                // End of directory
                if (e->name[0] == 0x00)
                    return NULL;

                // Deleted entry
                if ((uint8_t)e->name[0] == 0xE5)
                    continue;

                // Long filename entry
                if (e->attr == 0x0F)
                    continue;

                if (memcmp(e->name, name, 11))
                {
                    printf("Name: %s, not ename: %s\n", e->name, name);
                    File * f = malloc(sizeof(File));

                    uint32_t first_cluster = (e->first_cluster_high << 16) | e->first_cluster_low;

                    printf("Esize: %d\n", e->file_size);

                    f->first_cluster   = first_cluster;
                    f->current_cluster = first_cluster;
                    f->size            = e->file_size;
                    f->position        = 0;



                    return f;
                }
            }
        }

        // Move to next cluster in directory
        cluster = fat_next_cluster(fs, cluster);
    }

    return NULL;
}

uint32_t fs_read(FileSystem *fs, File *f, void *buffer, uint32_t size)
{
    uint8_t * out = buffer;
    uint32_t bytes_read = 0;

    while (bytes_read < size && f->position < f->size)
    {
        uint32_t sector = cluster_to_sector(fs, f->current_cluster);

        for (uint64_t s = 0; s < fs->sectors_per_cluster; s++)
        {
            uint8_t *src = fs->disk + (sector + s) * fs->bytes_per_sector;

            for (int i = 0; i < fs->bytes_per_sector; i++)
            {
                if (bytes_read >= size || f->position >= f->size)
                    return bytes_read;

                out[bytes_read++] = src[i];
                f->position++;
            }
        }

        // move to next cluster
        uint32_t next = fat_next_cluster(fs, f->current_cluster);

        if (next >= 0x0FFFFFF8)
            break;

        f->current_cluster = next;
    }

    return bytes_read;
}

void fs_init(uint64_t multibootinfo)
{
    uint64_t multibootinfo_virt = (uint64_t)multibootinfo + VIRT_BASE;

    multiboot_tag * tag;
    for (tag = (multiboot_tag *) (multibootinfo_virt + 8); 
    tag->type != 0; 
    tag = (multiboot_tag *) ((uint8_t *) tag + ((tag->size + 7) & ~7))) 
    {
        if (tag->type == 3)
        {
            multiboot_tag_module * module = (multiboot_tag_module *) tag;

            uint8_t * disk = (uint8_t *)module->mod_start + VIRT_BASE;
            uint8_t *sector0 = disk;
            fat32_bootsector *bs = (fat32_bootsector*)sector0;

            if (sector0[510] != 0x55 || sector0[511] != 0xAA) 
            {
                printf("Not a bootable disk");
            }

            fs.bytes_per_sector    = bs->bytes_per_sector;
            fs.sectors_per_cluster = bs->sectors_per_cluster;
            fs.reserved_sectors    = bs->reserved_sectors;
            fs.num_fats            = bs->num_fats;
            fs.fat_size            = bs->fat_size_32;
            fs.root_cluster        = bs->root_cluster;
            fs.disk                = (uint8_t*)disk;
            fs.fat_start = bs->reserved_sectors;
            fs.data_start = bs->reserved_sectors + (bs->num_fats * bs->fat_size_32);


            File *f = fs_open(&fs, "USER    ELF");

            // uint8_t size = 100;
            char buffer[f->size + 1];

            uint32_t bytes_read = fs_read(&fs, f, buffer, f->size + 1);


            elf_ehdr_t *elf = (elf_ehdr_t*)buffer;

            printf("\n");
            print_char((char)elf->iden_bytes[0]);
            print_char((char)elf->iden_bytes[1]);
            print_char((char)elf->iden_bytes[2]);
            print_char((char)elf->iden_bytes[3]);

            printf("\n");


            if (elf->iden_bytes[0] != 0x7F ||
                elf->iden_bytes[1] != 'E' ||
                elf->iden_bytes[2] != 'L' ||
                elf->iden_bytes[3] != 'F')
            {
                printf("\n");
            }

            elf_phdr_t *ph = (elf_phdr_t*)(buffer + elf->e_phoff);

            for (int i = 0; i < elf->e_phnum; i++)
            {
                if (ph[i].p_type != 1) // PT_LOAD 
                    continue;
                    



                // for(uint8_t i = 0; i < pages; i++)
                // {
                //     uint8_t * addr = (uint8_t *)(virtual_address + (i * 0x1000));

                //     printf("addr: %x\n", addr);

                //     vmm_translate(addr);
                //     // printf("d: %d\n", (*addr));
                // }


                // printf("filez: %d\n", ph[i].p_filesz);

                // printf("offset: %d\n", ph[i].p_offset);

                // memcpy(virtual_address, buffer + ph[i].p_offset, ph[i].p_filesz);

                // for(uint8_t i = 0; i < 4; i++)
                // {
                //     printf("addr: %x\n", ((uint64_t*)virtual_address)[i]);
                // }
                // // free
                // // uint64_t * frames = pmm_alloc(size, frame_nums);
                // // print_clear();
                // // if (frames != NULL)
                // // {
                // //     for(uint64_t i = 0; i < frame_nums; i++)
                // //     {
                // //         printf("frames[%d] %x\n", i, frames[i]);
                // //     }
                // // }
                
                // // // zero BSS
                // // memset(dest + ph[i].filesz,
                // //     0,
                // //     ph[i].memsz - ph[i].filesz);

                // uint64_t virt_stack = USER_STACK_START;
                // uint64_t stack_pages = (0x200000 + 0x1000 - 1) / 0x1000;

                // printf("%d\n", stack_pages);
                // uint64_t * phys_start = malloc(sizeof(uint64_t) * stack_pages);
                // pmm_alloc(stack_pages, phys_start);
                // uint64_t virt_stack_region = phys_stack_region + VIRT_BASE;
                // memset(virt_stack_region, 0, 0x200000);

                // map_pages(virt_stack, phys_start, stack_pages , PAGE_PRESENT | PTE_W | PTE_U);

                // vmm_translate(virt_stack + 0x1000 * 200);
                // printf("addr: %x\n", ((uint64_t*)virt_stack)[200]);

                // for(uint8_t i = 0; i < 4; i++)
                // {
                    
                // }


                                uint64_t pages = (0x200000 + PAGE_SIZE - 1) / 0x1000;
                uint64_t virtual_address = ph[i].p_vaddr;
                
                uint64_t count =  count_frames();

                 printf("before: %d\n", count);
                uint64_t * frames = pmm_alloc(pages);

                printf("FInished allocing\n");

                count = count_frames();

                printf("after: %d\n", count);

                // printf("%d\n", pmm_set_bit(2093056));

                map_pages(virtual_address, frames, pages, PAGE_PRESENT | PTE_W | PTE_U);

                uint8_t * addr = (uint8_t *)(virtual_address + ((pages - 1) * PAGE_SIZE));
                vmm_translate(addr);

                // // set the first mem length of p_memsz to 0
                memset(virtual_address, 0, ph[i].p_memsz);

                free(frames);

            }
        }
        
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
       
        if (tag->type == 6) 
        {
            multiboot_tag_mmap * mmap = (multiboot_tag_mmap *) tag;
            parse_mmap(mmap);
        }        
    }

	// uint64_t number_regions = 0;
	MemoryRegion * current = memoryRegions;

	while (current != NULL)
	{
		// number_regions++;
        // if (current->addr != 0)
        // {
            total_memory_size += current->size;

            printf("size men: %d\n", current->size);
        // }
		current = current->next;
	}

	total_pages = total_memory_size / PAGE_SIZE;
	total_frames = total_pages;

    printf("total pages: %d\n", total_pages);

    // Calculate bitmap size (ceil it up to nearest byte)
    bitmap_size_bytes = (total_pages + 7) / 8;

    printf("bitmap_size_bytes: %d\n", bitmap_size_bytes);

    // align with 4096 kb size so that when we mark the memory as used it doesn't reallocate excess bits
    bitmap_size_bytes = (bitmap_size_bytes + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);


    bitmap = (uint64_t *) malloc(bitmap_size_bytes);

    num_64_bits_needed = (bitmap_size_bytes + 7) >> 3;

    for (uint64_t i = 0; i < num_64_bits_needed; i++)
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

            // print_clear();

            printf("Start: %x\n", start);
            printf("End: %x\n", end);
            printf("Start: %d\n", first_frame);
            printf("End: %d\n", last_frame);
            printf("frame nums: %d\n", last_frame - first_frame);
        // printf("Kernel start: %x\n", ((uint64_t)__kernel_start) - VIRT_BASE);
        // printf("Kernel end: %x\n", ((uint64_t)__kernel_end) - VIRT_BASE);
            for (uint64_t f = first_frame; f < last_frame; f++)
            {
                clear_frame(f);
            }	                
		}
		current = current->next;
	}

	uint64_t kernel_heap_start = __kernel_start - VIRT_BASE;
	uint64_t kernel_heap_end = __kernel_end - VIRT_BASE;
	uint64_t first_reserved = kernel_heap_start / PAGE_SIZE;
    uint64_t last_reserved  = (kernel_heap_end + PAGE_SIZE - 1) / PAGE_SIZE;


    printf("pages needed for kernel %d\n", last_reserved - first_reserved);

	for (uint64_t f = first_reserved; f < last_reserved; f++)
	{
		set_frame(f);
	}

    printf("frame count: %d\n", count_frames());
}

uint64_t * pmm_alloc(uint64_t size)
{  
    uint64_t * buffer = malloc(sizeof(uint64_t) * size);

    // buffer[0] = 0xDEADBEEF;
    // return buffer;
    uint64_t counter = 0;
    for (uint64_t i = 0; i < num_64_bits_needed; i++)
    {
        if (bitmap[i] != 0xFFFFFFFFFFFFFFFFULL)
        {
            for (uint64_t bit = 0; bit < 64; bit++)
            {
                uint64_t frame = i * 64 + bit;

                if (! pmm_get_bit(frame))
                {
                    set_frame(frame); 

                    buffer[counter] = (uint64_t) (frame * PAGE_SIZE);

                    counter++;

                    if (counter >= size)
                    {
                        return buffer;
                    }
                    // return (uint64_t *) (frame * PAGE_SIZE);
                }
            }
        }
    }

    if (counter >= size)
    {
        return buffer;
    }

    return NULL;  // out of memory
}


uint64_t count_frames(void)
{

    uint64_t count = 0;
    uint8_t index = 0;
    for (uint64_t i = 0; i < num_64_bits_needed; i++)
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
                    
                if (! pmm_get_bit(frame))
                {
                    count++;
                }
            }
        }
    }

    return count;  // out of memory
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
                    
                if (!pmm_get_bit(frame))
                {
                    set_frame(frame); 
                    return (uint64_t *) (frame * PAGE_SIZE);
                }
            }
        }
    }

    return NULL;  // out of memory
}

uint32_t cluster_to_sector(FileSystem *fs, uint32_t cluster)
{
    return fs->data_start +
           (cluster - 2) * fs->sectors_per_cluster;
}

uint32_t fat_next_cluster(FileSystem *fs, uint32_t cluster)
{
    uint32_t fat_offset = cluster * 4;

    uint32_t fat_sector = fs->fat_start + (fat_offset / 512);

    uint32_t *fat = (uint32_t*)(fs->disk + fat_sector * 512);

    return fat[cluster] & 0x0FFFFFFF;
}