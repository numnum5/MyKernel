#include "x86_64/list.h"
#include "x86_64/pmm.h"
#include "x86_64/stdlib.h"
#include "x86_64/paging.h"
#include "x86_64/string.h"
#include "print.h"

static MemoryRegion * memoryRegions = NULL;
static uint64_t * bitmap = NULL;
static uint64_t total_memory_size = 0;
static uint64_t total_pages = 0;
static uint64_t total_frames;
static uint64_t bitmap_size_bytes;
FileSystem fs;


uint32_t current_cluster;
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

void wrapper(void)
{
    fat32_mkdir(fs.root_cluster, "test");
    fat32_mkdir(fs.root_cluster, "sys");
    // fat32_mkdir(fs.root_cluster, "sys");
}

void fat32_mkdir(uint32_t parent_cluster, const char *name)
{
    uint32_t new_cluster = fat32_find_free_cluster();

    fat32_init_directory(&fs, new_cluster, parent_cluster);

    fat32_add_entry(&fs, parent_cluster, name, new_cluster);
}


void fat32_add_entry(FileSystem *fs,
                     uint32_t parent_cluster,
                     const char *name,
                     uint32_t new_cluster)
{
    uint32_t cluster = parent_cluster;

    while (cluster < 0x0FFFFFF8)
    {
        uint32_t sector = cluster_to_sector(fs, cluster);

        fat_dir_entry *dir =
            (fat_dir_entry*)(fs->disk + sector * fs->bytes_per_sector);

        uint32_t entries =
            fs->bytes_per_sector / sizeof(fat_dir_entry);

        for (uint32_t i = 0; i < entries; i++)
        {
            if (dir[i].name[0] == 0x00 || dir[i].name[0] == 0xE5)
            {
                memset(&dir[i], 0, sizeof(fat_dir_entry));
                memset(dir[i].name, ' ', 11);
                memcpy(dir[i].name, name, 11);

                dir[i].attr = 0x10;
                dir[i].first_cluster_high = new_cluster >> 16;
                dir[i].first_cluster_low = new_cluster & 0xFFFF;

                return;
            }
        }

        cluster = fat_next_cluster(fs, cluster);
    }
}


void fat32_init_directory(FileSystem *fs,
                          uint32_t cluster,
                          uint32_t parent)
{
    uint32_t sector = cluster_to_sector(fs, cluster);


    
    uint8_t *buf = fs->disk + sector * fs->bytes_per_sector;

    memset(buf, 0, fs->bytes_per_sector); // sectors_per_cluster = 1

    fat_dir_entry *e = (fat_dir_entry*)buf;

    // "."
    memcpy(e[0].name, ".", 11);
    e[0].attr = 0x10;
    e[0].first_cluster_high = cluster >> 16;
    e[0].first_cluster_low = cluster & 0xFFFF;

    // ".."
    memcpy(e[1].name, "..", 11);
    e[1].attr = 0x10;
    e[1].first_cluster_high = parent >> 16;
    e[1].first_cluster_low = parent & 0xFFFF;
}

uint32_t fat32_find_free_cluster(void)
{
    // printf("fat_size: %d\n", fs.fat_size);
    // printf("fat size 2: %d\n", fs.bytes_per_sector);

    // printf("sectors per cluster: %d\n", fs.sectors_per_cluster);
    uint32_t fat_entries = (fs.fat_size * fs.bytes_per_sector) / 4;

    uint32_t *fat = (uint32_t*)(fs.disk + fs.fat_start * fs.bytes_per_sector);

    for (uint32_t cluster = 0; cluster < fat_entries; cluster++)
    {

        if ((fat[cluster] & 0x0FFFFFFF) == 0)
        {
            fat[cluster] = 0x0FFFFFFF;
            // printf("cluster: %d\n", cluster);
            return cluster;
        }
    }

    return 0; // no free cluster
}




void cd_dir(uint32_t cluster_dir, const char * name)
{
    FileSystem * fileSystem = &fs;
    uint32_t cluster = cluster_dir;

    while (cluster < 0x0FFFFFF8)
    {
        uint32_t sector = cluster_to_sector(fileSystem, cluster);
        // For each sector in this cluster
        for (uint32_t s = 0; s < fileSystem->sectors_per_cluster; s++)
        {
            fat_dir_entry *dir = (fat_dir_entry*) (fileSystem->disk + (sector + s) * fileSystem->bytes_per_sector);

            uint8_t entries_per_sector = fileSystem->bytes_per_sector / sizeof(fat_dir_entry);
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

                if (e->attr == 0x10)
                {   

                    
                    if (strcmp(e->name, name) == 0)
                    {   
            
                        current_cluster = (e->first_cluster_high << 16) | e->first_cluster_low;
                        // printf("Names: %s, %s, Cluster: %x\n", e->name, name, current_cluster);
                        // printf("%x\n", current_cluster);
                    }
                }
            }
        }

        // Move to next cluster in directory
        cluster = fat_next_cluster(fileSystem, cluster);
    }
}


char * get_dir_name(uint32_t parent, uint32_t child)
{
    uint32_t sector = cluster_to_sector(&fs, parent);
    for (uint32_t s = 0; s < fs.sectors_per_cluster; s++)
    {
        fat_dir_entry *dir = (fat_dir_entry*) (fs.disk + (sector + s) * fs.bytes_per_sector);

        uint8_t entries_per_sector = fs.bytes_per_sector / sizeof(fat_dir_entry);

        for (uint8_t i = 0; i < entries_per_sector; i++)
        {
            fat_dir_entry *e = &dir[i];

            // // End of directory
            if (e->name[0] == 0x00)
                continue;

            // Deleted entry
            if ((uint8_t)e->name[0] == 0xE5)
                continue;
            //                 // Long filename entry
            if (e->attr == 0x0F)
                continue;

            if (e->attr == 0x10)
            {
                // vga_print(e->name);
                uint32_t selected_cluster = (e->first_cluster_high << 16) | e->first_cluster_low;

                if (selected_cluster == child)
                {
                    char * dir_name = malloc(sizeof(char) * 13);
                    strcpy(dir_name, e->name);
                    return dir_name;
                }              
            }
        }
    }

    return NULL;
}

uint8_t pwd(uint32_t cluster_dir, char ** names)
{
 
    uint8_t index = 0;
    if (cluster_dir == fs.root_cluster) {
        names[index] = malloc(sizeof(char) * 13);
        strcpy(names[index], "");
        return index;
    }

    uint32_t child = cluster_dir;;
    uint32_t parent;

    do
    {
        parent = search_entry(child, "..");
        char * dir_name = get_dir_name(parent, child); 
        names[index++] = dir_name;
        child = parent;
    }
    while(parent != fs.root_cluster);


    return index;
}

char * list_current_dir(uint32_t cluster_dir)
{
    static char name[13]; // 8.3 + null

    // Root special case
    if (cluster_dir == fs.root_cluster) {
        strcpy(name, "");
        return name;
    }

    uint32_t parent = search_entry(cluster_dir, "..");

    // printf("%x\n", parent);
    FileSystem * fileSystem = &fs;
    uint32_t sector = cluster_to_sector(fileSystem, parent);


    // // For each sector in this cluster
    for (uint32_t s = 0; s < fileSystem->sectors_per_cluster; s++)
    {
        fat_dir_entry *dir = (fat_dir_entry*) (fileSystem->disk + (sector + s) * fileSystem->bytes_per_sector);
    // }
        uint8_t entries_per_sector = fileSystem->bytes_per_sector / sizeof(fat_dir_entry);
        // printf("sector: %d\n", entries_per_sector);

        for (uint8_t i = 0; i < entries_per_sector; i++)
        {
            fat_dir_entry *e = &dir[i];

            // // End of directory
            if (e->name[0] == 0x00)
                continue;

            // Deleted entry
            if ((uint8_t)e->name[0] == 0xE5)
                continue;
            //                 // Long filename entry
            if (e->attr == 0x0F)
                continue;

            if (e->attr == 0x10)
            {
                // vga_print(e->name);
               uint32_t selected_cluster = (e->first_cluster_high << 16) | e->first_cluster_low;

                if (selected_cluster == cluster_dir)
                {
                    strcpy(name, e->name);
                    return name;
                }              
            }
        }
    }

    return "?";
}

#define NO_ENTRY_ERR 0

uint32_t search_entry(uint32_t cluster_dir, char * entry_name)
{
    FileSystem * fileSystem = &fs;
    uint32_t sector = cluster_to_sector(fileSystem, cluster_dir);
    
    // For each sector in this cluster

    for (uint32_t s = 0; s < fileSystem->sectors_per_cluster; s++)
    {
        fat_dir_entry *dir = (fat_dir_entry*) (fileSystem->disk + (sector + s) * fileSystem->bytes_per_sector);

        uint8_t entries_per_sector = fileSystem->bytes_per_sector / sizeof(fat_dir_entry);
        // printf("sector: %d\n", entries_per_sector);

        for (uint8_t i = 0; i < entries_per_sector; i++)
        {
            fat_dir_entry *e = &dir[i];

            // End of directory
            if (e->name[0] == 0x00)
                continue;

            // Deleted entry
            if ((uint8_t)e->name[0] == 0xE5)
                continue;
                            // Long filename entry
            if (e->attr == 0x0F)
                continue;

            if (e->attr == 0x10)
            {

                // vga_print(e->name);
                if (strcmp(e->name, entry_name) == 0)
                {
                    uint32_t selected_cluster = (e->first_cluster_high << 16) | e->first_cluster_low;
                    return selected_cluster;
                }                
            }
        }
    }

    return NO_ENTRY_ERR;
}




/// root -> [sys : [], test : [ file file file etc...]]
void list_dir(uint32_t cluster_dir)
{   
    FileSystem * fileSystem = &fs;
    uint32_t cluster = cluster_dir;

    while (cluster < 0x0FFFFFF8)
    {
        uint32_t sector = cluster_to_sector(fileSystem, cluster);
        // For each sector in this cluster
        for (uint32_t s = 0; s < fileSystem->sectors_per_cluster; s++)
        {
            fat_dir_entry *dir = (fat_dir_entry*) (fileSystem->disk + (sector + s) * fileSystem->bytes_per_sector);

            uint8_t entries_per_sector = fileSystem->bytes_per_sector / sizeof(fat_dir_entry);
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

                if (e->attr == 0x10)
                {
                    print_set_color(PRINT_COLOR_YELLOW, PRINT_COLOR_BLACK);
                    printf(e->name);
                    print_char('\n');                   
                }
            }
        }

        // Move to next cluster in directory
        cluster = fat_next_cluster(fileSystem, cluster);
    }
}

void fat32_list(FileSystem *fs)
{
    uint32_t cluster = fs->root_cluster;

    while (cluster < 0x0FFFFFF8)
    {
        uint32_t sector = cluster_to_sector(fs, cluster);
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

                if (e->attr == 0x10)
                {
                    vga_print(e->name);
                    vga_putc('\n');                   
                }
            }
        }

        // Move to next cluster in directory
        cluster = fat_next_cluster(fs, cluster);
    }
}


void fat32_list_wrapper(void)
{
    fat32_list(&fs);
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

            // Intialise current directory to keep track for shell;
            current_cluster = fs.root_cluster;
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