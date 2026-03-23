#include "print.h"
#include "keyboard.h"
#include "x86_64/rtc.h"
#include "x86_64/interrupts.h"
#include "x86_64/pit.h"
#include "x86_64/memory.h"
#include "x86_64/list.h"
#include "x86_64/paging.h"
// #include "x86_64/multiboot.h"

#define KEY_CODE_A 0x1E
#define KEY_CODE_B 0x30
#define KEY_CODE_C 0x2E
#define KEY_CODE_D 0x20
#define KEY_CODE_E 0x12
#define KEY_CODE_F 0x21
#define KEY_CODE_G 0x22
#define KEY_CODE_H 0x23
#define KEY_CODE_I 0x17
#define KEY_CODE_J 0x24
#define KEY_CODE_K 0x25
#define KEY_CODE_L 0x26
#define KEY_CODE_M 0x32
#define KEY_CODE_N 0x31
#define KEY_CODE_O 0x18
#define KEY_CODE_P 0x19
#define KEY_CODE_Q 0x10
#define KEY_CODE_R 0x13
#define KEY_CODE_S 0x1F
#define KEY_CODE_T 0x14
#define KEY_CODE_U 0x16
#define KEY_CODE_V 0x2F
#define KEY_CODE_W 0x11
#define KEY_CODE_X 0x2D
#define KEY_CODE_Y 0x15
#define KEY_CODE_Z 0x2C
#define KEY_CODE_SPACE 0x39
#define KEY_CODE_ENTER 0x1C

char to_ascii(uint16_t code) {
    switch (code) {
        case KEY_CODE_A: return 'A';
        case KEY_CODE_B: return 'B';
        case KEY_CODE_C: return 'C';
        case KEY_CODE_D: return 'D';
        case KEY_CODE_E: return 'E';
        case KEY_CODE_F: return 'F';
        case KEY_CODE_G: return 'G';
        case KEY_CODE_H: return 'H';
        case KEY_CODE_I: return 'I';
        case KEY_CODE_J: return 'J';
        case KEY_CODE_K: return 'K';
        case KEY_CODE_L: return 'L';
        case KEY_CODE_M: return 'M';
        case KEY_CODE_N: return 'N';
        case KEY_CODE_O: return 'O';
        case KEY_CODE_P: return 'P';
        case KEY_CODE_Q: return 'Q';
        case KEY_CODE_R: return 'R';
        case KEY_CODE_S: return 'S';
        case KEY_CODE_T: return 'T';
        case KEY_CODE_U: return 'U';
        case KEY_CODE_V: return 'V';
        case KEY_CODE_W: return 'W';
        case KEY_CODE_X: return 'X';
        case KEY_CODE_Y: return 'Y';
        case KEY_CODE_Z: return 'Z';
        case KEY_CODE_SPACE: return ' ';
        case KEY_CODE_ENTER: return '\n';
    }    
    
    return '?';
}

void handle_input(struct KeyboardEvent event) {
    if (event.type == KEYBOARD_EVENT_TYPE_MAKE) {
        print_set_color(PRINT_COLOR_BLUE, PRINT_COLOR_WHITE);
        print_char(to_ascii(event.code));
    } else if (event.type == KEYBOARD_EVENT_TYPE_BREAK) {
    }
}

#define IDT_IRQ0_TIMER 0x20
#define IDT_IRQ1_KEYBOARD 0x21
#define IDT_GATE_PRESENT (1 << 7)
#define IDT_GATE_DPL0 (0b00 << 5)
#define IDT_GATE_DPL1 (0b01 << 5)
#define IDT_GATE_DPL2 (0b10 << 5)
#define IDT_GATE_DPL3 (0b11 << 5)
#define IDT_GATE_TYPE_INTERRUPT 0xE
#define IDT_ENTRY_TYPE_INTERRUPT (IDT_GATE_PRESENT | IDT_GATE_DPL0 | IDT_GATE_TYPE_INTERRUPT)
#define VIRT_BASE 0xffffffff80000000



struct multiboot_tag {
    uint32_t type;
    uint32_t size;
};

struct multiboot_mmap_entry {
    uint64_t addr;
    uint64_t len;
#define MULTIBOOT_MEMORY_AVAILABLE              1
#define MULTIBOOT_MEMORY_RESERVED               2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE       3
#define MULTIBOOT_MEMORY_NVS                    4
#define MULTIBOOT_MEMORY_BADRAM                 5
    uint32_t type;
    uint32_t zero;
};

struct multiboot_tag_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    struct multiboot_mmap_entry entries[];
};


uint64_t get_cr3() {
    uint64_t val;
    asm volatile("mov %%cr3, %0" : "=r"(val));
    return val;
}

extern char __kernel_heap_start;
extern char __kernel_heap_end;


// extern uint8_t ucHeap;
extern uint64_t page_table_l4;
uintptr_t kernel_start = (uintptr_t)&__kernel_heap_start;
uintptr_t kernel_end   = (uintptr_t)&__kernel_heap_end;
// uintptr_t t   = (uintptr_t)&page_table_l4;

void parse_mmap(struct multiboot_tag_mmap *mmap_tag);


MemoryRegion * memoryRegions = NULL;

uint64_t get_l4_page_table(void)
{
    uint64_t cr3;
    asm volatile ("mov %%cr3, %0" : "=r"(cr3));
    return cr3 & 0x000FFFFFFFFFF000ULL;
}

#define PAGE_PRESENT  (1ULL << 0)
#define PAGE_HUGE     (1ULL << 7)
#define ADDRESS_MASK  0x000FFFFFFFFFF000ULL 

typedef enum {
    PAGE_SIZE_4KB,
    PAGE_SIZE_2MB,
    PAGE_SIZE_1GB,
    PAGE_NOT_PRESENT
} page_info_t;

page_info_t get_page_size(uint64_t pml4_phys, uint64_t virt_addr) {
    // 1. Extract indices for each level from the virtual address
    uint64_t pml4_idx = (virt_addr >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_idx   = (virt_addr >> 21) & 0x1FF;
    // (pt_idx is at >> 12, but we only need it if it's not a huge page)

    // 2. Access PML4 (Level 4)
    uint64_t* pml4 = (uint64_t*)pml4_phys;
    if (!(pml4[pml4_idx] & PAGE_PRESENT)) return PAGE_NOT_PRESENT;

    // 3. Access PDPT (Level 3)
    uint64_t* pdpt = (uint64_t*)(pml4[pml4_idx] & ADDRESS_MASK);
    if (!(pdpt[pdpt_idx] & PAGE_PRESENT)) return PAGE_NOT_PRESENT;
    
    // Check for 1GB Huge Page at Level 3
    if (pdpt[pdpt_idx] & PAGE_HUGE) return PAGE_SIZE_1GB;

    // 4. Access PD (Level 2)
    uint64_t* pd = (uint64_t*)(pdpt[pdpt_idx] & ADDRESS_MASK);
    if (!(pd[pd_idx] & PAGE_PRESENT)) return PAGE_NOT_PRESENT;

    // Check for 2MB Huge Page at Level 2
    if (pd[pd_idx] & PAGE_HUGE) return PAGE_SIZE_2MB;

    // 5. If we reached here and it's present, it's a standard 4KB page
    return PAGE_SIZE_4KB;
}


void test(void)
{

}

void test2(void)
{
    
}

void kernel_main(uint32_t magic, void * addr) 
{
    print_clear();
    print_set_color(PRINT_COLOR_YELLOW, PRINT_COLOR_BLACK);
    print_str("Welcome to our 64-bit kernel!\n");
    uint64_t  pml4 = get_l4_page_table();

    pit_init();
    idt_initv2();
    scheduler_init();

    create_thread(test, 10000);
    create_thread(test2, 10000);

    start_scheduler();


    // prvHeapInit();

    // MemoryRegion * ptr = pvPortMalloc(sizeof(MemoryRegion));

    // ptr->addr = 
    // print_memory_info();
    // MemoryRegion * ptr =  (MemoryRegion *) kmalloc(sizeof(MemoryRegion));


    // if (ptr != NULL)
    // {
    //     ptr->addr = 0xDEADBEEF;
    //     ptr->size = 555;
    //     print_char('\n');
    //     print_uint64_hex( ptr->addr);
    //     print_char('\n');
    //     print_uint64_hex( ptr->size);
    //     print_char('\n');
    //     // print_memory_info(); 

    // }

    
    // uint64_t * ptr = pvPortMalloc(sizeof(uint64_t));

    // (*ptr) = 0xDEADBEEF;


    // uint64_t phys = ((uint64_t) ptr) - VIRT_BASE;


    // uint64_t v = *((uint64_t *) phys);
    //     print_str(" \n phys \n:");
    //     print_uint64_hex(v);
    //     print_char('\n');

//     uint64_t * ptr = pvPortMalloc(sizeof(uint64_t));
//     page_info_t value = get_page_size(pml4, (uint64_t) ptr);
//     uint64_t page_size = 0x1000;

//     switch (value)
//     {
//         case PAGE_SIZE_4KB:
//             print_str("4kb\n");
//             break;
//         case PAGE_SIZE_2MB:
//             print_str("2mb\n");
//             page_size = 0x200000;
//             break;
//         case PAGE_SIZE_1GB:
//             print_str("1gb\n");
//             break;
//         case PAGE_NOT_PRESENT:
//             print_str("not present\n");
//             break;
//     }
//     //addr is the physical address of the multiboot structure
//     struct multiboot_tag *tag;
// //
//    // Loop through tags starting at addr + 8 (skipping the total_size and reserved fields)
//     for (tag = (struct multiboot_tag *) (addr + 8);
//          tag->type != 0; // Type 0 is the end tag
//          tag = (struct multiboot_tag *) ((uint8_t *) tag + ((tag->size + 7) & ~7))) 
//     {
//         // Type 6 is the Memory Map tag
//         if (tag->type == 6) {
//             struct multiboot_tag_mmap *mmap = (struct multiboot_tag_mmap *) tag;
//             parse_mmap(mmap);

//         }
//     }

//     (void)pmm_init();

    // each frame is mapped to a valid physical address 
    // uint64_t * frame = pmm_alloc_frame();

    // print_str("\n FRAME: \n");
	// print_uint64_hex(frame);
	// print_char('\n');
// printNodes(memoryRegions);


    // keyboard_init();
    // keyboard_set_handler(handle_input);
    
    // uint8_t prev_seconds = 0;
    
    // for (uint8_t i = 0; i < 5;) {
    //     uint8_t seconds = rtc_seconds();
        
    //     if (seconds != prev_seconds) {
    //         i++;
    //         print_set_color(PRINT_COLOR_GREEN, PRINT_COLOR_BLACK);
    //         print_str("\nSeconds: ");
    //         print_uint64_dec(seconds);
    //     }
        
    //     prev_seconds = seconds;
    // }
    
    // print_str(" - Seconds loop disabled.\n");

    // vPortFree(ptr);

    while (1);
}





void parse_mmap(struct multiboot_tag_mmap *mmap_tag) {
    // Calculate how many entries are in this tag
    // (Tag size - header size) / entry size
    uint32_t num_entries = (mmap_tag->size - sizeof(struct multiboot_tag_mmap)) 
                           / mmap_tag->entry_size;

    for (uint32_t i = 0; i < num_entries; i++) {
        struct multiboot_mmap_entry *entry = &mmap_tag->entries[i];

        // Print or process the entry
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
            // This is the RAM you can use for your Page Frame Allocator
            // printf("Available RAM: Start: 0x%llx, Length: 0x%llx\n", entry->addr, entry->len);

            MemoryRegion * region = pvPortMalloc(sizeof(MemoryRegion));

            region->addr = entry->addr;
            region->size = entry->len;
            region->type = entry->type;

            push_back(&memoryRegions, region);

            // print_str("\ntype: ");
            // print_uint64_dec(entry->type);
            // print_str("\nsize: ");
            // print_uint64_dec(entry->len);
            // print_str("\naddr: ");
            // print_uint64_hex( (uint64_t) entry->addr + VIRT_BASE);
            // print_str("\n ");
        } else {
            MemoryRegion * region = pvPortMalloc(sizeof(MemoryRegion));

            region->addr = entry->addr;
            region->size = entry->len;
            region->type = entry->type;

            push_back(&memoryRegions, region);
            // Reserved or ACPI memory - stay away!
        }
    }
}