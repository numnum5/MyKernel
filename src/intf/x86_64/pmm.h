
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
#define USER_STACK 0x7FFFFFFFFFF0 // Alignment
#define USER_STACK_START (USER_STACK - 0x200000 + 16)
// #define USER_STACK_START (0x7FFFFFFFFFF0 - 0x200000)
#define USER_STACK_TOP 0x7FFFFFFFFFF0


typedef struct 
{
    uint32_t type;
    uint32_t size;
} multiboot_tag;

typedef struct {
    uint32_t type;   // = 3 (MULTIBOOT_TAG_TYPE_MODULE)
    uint32_t size;   // total size of this tag (including header)

    uint32_t mod_start;  // physical start address of module (disk.img in RAM)
    uint32_t mod_end;    // physical end address

    char cmdline[];     // optional null-terminated string (e.g. "disk")
} __attribute__((packed)) multiboot_tag_module;

typedef struct {
    uint8_t  jmp[3];
    char     oem[8];

    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  num_fats;

    uint16_t root_entries;        // FAT12/16 only (should be 0 for FAT32)
    uint16_t total_sectors_16;

    uint8_t  media;
    uint16_t fat_size_16;         

    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;

    uint32_t fat_size_32;         // THIS is key for FAT32
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;        // FAT32 root starts here

    uint16_t fs_info;
    uint16_t backup_boot_sector;

    uint8_t  reserved[12];

    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    char     volume_label[11];
    char     fs_type[8];          // "FAT32   "
} __attribute__((packed)) fat32_bootsector;


typedef struct {
    char name[11];
    uint8_t attr;
    uint8_t _ntres;
    uint8_t _crt_time_tenth;
    uint16_t _crt_time;
    uint16_t _crt_date;
    uint16_t _acc_date;
    uint16_t first_cluster_high;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} __attribute__((packed)) fat_dir_entry;

typedef struct {
    uint32_t bytes_per_sector;
    uint32_t sectors_per_cluster;
    uint32_t reserved_sectors;
    uint32_t num_fats;
    uint32_t fat_size;
    uint32_t root_cluster;
    uint32_t fat_start;
    uint32_t data_start;
    uint8_t *disk;
} FileSystem;

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

typedef struct {
    uint32_t first_cluster;
    uint32_t current_cluster;
    uint32_t size;
    uint32_t position;
} File;

typedef struct elf_ehdr {
    uint8_t iden_bytes[16];
    uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	uint64_t e_entry;
	uint64_t e_phoff;
	uint64_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;
} __attribute__((packed)) elf_ehdr_t;

typedef struct elf_phdr {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} __attribute__((packed)) elf_phdr_t;

typedef struct elf_shdr {
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
} __attribute__((packed)) elf_shdr_t;


void pmm_init(uint64_t multibootinfo);
void fs_init(uint64_t multibootinfo);
uint64_t * pmm_alloc_frame(void);
uint64_t * pmm_alloc(uint64_t size);
uint64_t count_frames(void);
uint32_t cluster_to_sector(FileSystem *fs, uint32_t cluster);
uint32_t fat_next_cluster(FileSystem *fs, uint32_t cluster);
uint32_t fs_read(FileSystem *fs, File *f, void *buffer, uint32_t size);