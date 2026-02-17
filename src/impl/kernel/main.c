#include "print.h"
#include "keyboard.h"
#include "x86_64/rtc.h"
#include "x86_64/interrupts.h"
#include "x86_64/pit.h"
#include "x86_64/memory.h"
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

uintptr_t kernel_start = (uintptr_t)&__kernel_heap_start;
uintptr_t kernel_end   = (uintptr_t)&__kernel_heap_end;


void parse_mmap(struct multiboot_tag_mmap *mmap_tag);
void kernel_main(uint32_t magic, void * addr) 
{
// void kernel_main() {

    print_clear();
    print_set_color(PRINT_COLOR_YELLOW, PRINT_COLOR_BLACK);
    print_str("Welcome to our 64-bit kernel!\n");
    print_str("\nSeconds: ");
    print_str("\nSeconds: ");
    print_uint64_hex(IDT_ENTRY_TYPE_INTERRUPT);
    print_str("\nWelcome to our 64-bit kernel!\n");

    // pit_init();
    // idt_initv2();

    print_uint64_hex(kernel_start);

    print_char('\n');
print_uint64_hex(kernel_end);


    uint64_t heap_size = kernel_end - kernel_start;

    print_char('\n');
print_uint64_hex(heap_size);

    heap_init(kernel_start, heap_size);



    uint64_t * ptr =  kmalloc(100);


    if (ptr != NULL)
    {
            print_char('\n');
    print_uint64_hex(ptr);
        
        *ptr = 0xDEADBEEF;
        print_uint64_hex(*ptr);

    }




    


    // heap_init(kernel_start)
//         volatile uint64_t *p = (uint64_t*)0xffffffff80000000;
//         *p = 0xdeadbeef;
//         uint64_t value = *p;   

//     // multiboot_memory_map_t * entry = (multiboot_memory_map_t *)(bootInfo->mmap_addr);
// // addr is the physical address of the multiboot structure
//     struct multiboot_tag *tag;

//     // Loop through tags starting at addr + 8 (skipping the total_size and reserved fields)
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



    // uint64_t add = get_cr3();

    // print_char('\n');
    // print_uint64_hex(add);
    // printf (" size = 0x%x, base_addr = 0x%x%08x,"
    //         " length = 0x%x%08x, type = 0x%x\n",
    //         (unsigned) mmap->size,
    //         (unsigned) (mmap->addr >> 32),
    //         (unsigned) (mmap->addr & 0xffffffff),
    //         (unsigned) (mmap->len >> 32),
    //         (unsigned) (mmap->len & 0xffffffff),
    //         (unsigned) mmap->type);
//  print_uint64_dec(entry->type);

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
    
    print_str(" - Seconds loop disabled.\n");
    
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
            print_str("\ntype: ");
            print_uint64_dec(entry->type);
            print_str("\nsize: ");
            print_uint64_dec(entry->len);
            print_str("\naddr: ");
            print_uint64_hex(entry->addr);
            print_str("\n ");
        } else {
            // Reserved or ACPI memory - stay away!
        }
    }
}