#include "print.h"
#include "keyboard.h"
#include "x86_64/rtc.h"
#include "x86_64/interrupts.h"
#include "x86_64/pit.h"
#include "x86_64/memory.h"
#include "x86_64/list.h"
#include "x86_64/paging.h"
#include "x86_64/tss.h"
#include "x86_64/msr.h"
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



typedef struct {
    uint32_t type;
    uint32_t size;
} multiboot_tag;

typedef struct  {
    uint64_t addr;
    uint64_t len;
#define MULTIBOOT_MEMORY_AVAILABLE              1
#define MULTIBOOT_MEMORY_RESERVED               2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE       3
#define MULTIBOOT_MEMORY_NVS                    4
#define MULTIBOOT_MEMORY_BADRAM                 5
    uint32_t type;
    uint32_t zero;
} multiboot_mmap_entry;

typedef struct {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    multiboot_mmap_entry entries[];
} multiboot_tag_mmap;


uint64_t get_cr3() {
    uint64_t val;
    asm volatile("mov %%cr3, %0" : "=r"(val));
    return val;
}

extern char __kernel_heap_start;
extern char __kernel_heap_end;


// extern uint8_t ucHeap;
extern uint8_t stack_top[];
extern uint8_t stack_bottom[];

uintptr_t kernel_start = (uintptr_t)&__kernel_heap_start;
uintptr_t kernel_end   = (uintptr_t)&__kernel_heap_end;

void parse_mmap(multiboot_tag_mmap *mmap_tag);

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

#define PTE_W  (1ULL << 1)  // 0x002
#define PTE_U  (1ULL << 2)  // 0x004

void map_page2(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags) {
    uint64_t pml4_idx = (virt_addr >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_idx   = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_idx   = (virt_addr >> 12) & 0x1FF;

    uint64_t *pml4 = (uint64_t *)(get_l4_page_table() + VIRT_BASE);

    // Step 1: PML4 -> PDPT
    if (!(pml4[pml4_idx] & PAGE_PRESENT)) {
        uint64_t frame = pmm_alloc_frame();
        memset((void*)(frame + VIRT_BASE), 0, 4096);
        pml4[pml4_idx] = frame | PAGE_PRESENT | PTE_W | PTE_U;
    }
    uint64_t *pdpt = (uint64_t *)((pml4[pml4_idx] & ADDRESS_MASK) + VIRT_BASE);

    // Step 2: PDPT -> PD
    if (!(pdpt[pdpt_idx] & PAGE_PRESENT)) {
        uint64_t frame = pmm_alloc_frame();
        memset((void*)(frame + VIRT_BASE), 0, 4096);
        pdpt[pdpt_idx] = frame | PAGE_PRESENT | PTE_W | PTE_U;
    }
    uint64_t *pd = (uint64_t *)((pdpt[pdpt_idx] & ADDRESS_MASK) + VIRT_BASE);

    // Step 3: PD -> PT
    if (!(pd[pd_idx] & PAGE_PRESENT)) {
        uint64_t frame = pmm_alloc_frame();
        memset((void*)(frame + VIRT_BASE), 0, 4096);
        // FIX: Assign to pd[pd_idx], NOT pdpt!
        pd[pd_idx] = frame | PAGE_PRESENT | PTE_W | PTE_U;
    }
    uint64_t *pt = (uint64_t *)((pd[pd_idx] & ADDRESS_MASK) + VIRT_BASE);

    // Step 4: Final PT Entry
    // FIX: Set the flags passed into the function (or your hardcoded 0x7)
    pt[pt_idx] = (phys_addr & ADDRESS_MASK) | PAGE_PRESENT | PTE_W | PTE_U;

    asm volatile("invlpg (%0)" :: "r"(virt_addr) : "memory");
}


uint64_t * vmm_translate(uint64_t virt)
{
    uint64_t pml4_idx = (virt >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
    uint64_t pd_idx   = (virt >> 21) & 0x1FF;
    uint64_t pt_idx   = (virt >> 12) & 0x1FF;
    uint64_t offset = virt & 0x1FFFFF;


    printf("pml4: %x, pdpt: %x, pd: %x, pt: %x\n", pml4_idx, pdpt_idx, pd_idx, pt_idx);


    uint64_t * pml4 = (uint64_t *) (get_l4_page_table() + VIRT_BASE);
    
    if (!(pml4[pml4_idx] & PAGE_PRESENT)) 
    {
        return NULL;
    }
    uint64_t* pdpt = (uint64_t*)((pml4[pml4_idx] & ADDRESS_MASK) + VIRT_BASE);

    if (!(pdpt[pdpt_idx] & PAGE_PRESENT)) 
    {
        return NULL;
    }

    uint64_t* pd = (uint64_t*)((pdpt[pdpt_idx] & ADDRESS_MASK) + VIRT_BASE);
    if (!(pd[pd_idx] & PAGE_PRESENT)) {
        return NULL;
    }

    uint64_t* pt = (uint64_t*)((pd[pd_idx] & ADDRESS_MASK) + VIRT_BASE);

    if (!(pt[pt_idx] & PAGE_PRESENT)) 
    {
        return NULL;
    }

    uint64_t flags = pt[pt_idx] & 0xFFF;

    printf("PD Flags: %x\n", flags);

    return (uint64_t*)((pt[pt_idx] & ADDRESS_MASK) + offset);
}


void test_user_function(void)
{
    print_str("Hello from task2!\n");
    while (1){
        // for(volatile int i = 0; i < 100000000; i++);
    };

}

void test2(void)
{
    print_str("Hello from task34!\n");
    while (1){
        // for(volatile int i = 0; i < 100000000; i++);
    };
}

extern uint8_t stack_top[];
extern uint8_t stack_bottom[];
extern uint8_t gdt64[];


extern void jump_usermode();
extern void enter_user_mode(uint64_t user_rip, uint64_t user_rsp);
extern void start_first_thread(uint64_t * sp);
extern uint8_t udata_selector[];
extern uint8_t ucode_selector[];
void kernel_main(uint32_t magic, void * multibootinfo) 
{
    print_clear();
    print_set_color(PRINT_COLOR_YELLOW, PRINT_COLOR_BLACK);
    print_str("Welcome to our OS kernel!\n");
    init_TSS();
    idt_initv2();
    // init_syscall_interface();
    // pit_init();
    // scheduler_init();
    // Thread * thread = create_userthread(test2,0x1000, 0xDEEDBEEF);
    // create_thread(test,0x1000, 0xABCDEFFF);
    // start_scheduler();

    uint64_t multibootinfo_virt = (uint64_t)multibootinfo + VIRT_BASE;

    multiboot_tag * tag;
    for (tag = (multiboot_tag *) (multibootinfo_virt + 8); 
    tag->type != 0; 
    tag = (multiboot_tag *) ((uint8_t *) tag + ((tag->size + 7) & ~7))) 
    {
        // Type 6 is the Memory Map tag
        if (tag->type == 6) {
            multiboot_tag_mmap * mmap = (multiboot_tag_mmap *) tag;
            parse_mmap(mmap);


            printf("\n tag size %x\n", tag->type);
        }
    }

    pmm_init();


    uint64_t phys_stack = pmm_alloc_frame(); // 2MB
    uint64_t phys_stack1 = pmm_alloc_frame(); // 2MB
    uint64_t phys_stack2 = pmm_alloc_frame(); // 2MB

    uint64_t virt_stack = phys_stack + VIRT_BASE; // must be 2MB aligned

    // // 1. Map first
    map_page2(
        virt_stack,
        phys_stack,
        PAGE_PRESENT | PTE_U | PTE_W
    );

    uint64_t user_stack_top = (virt_stack + 0x500);

    vmm_translate(virt_stack);
    vmm_translate(user_stack_top);


    uint64_t *frame = (uint64_t *)(user_stack_top - 5 * 8);

    frame[0] = (uint64_t)test2;    
    frame[1] = 0x1B;               
    frame[2] = 0x202;              
    frame[3] = user_stack_top;  
    frame[4] = 0x23;               


    asm volatile (
        "mov %0, %%rsp\n"
        "ret\n"
        :
        : "r"(frame)
        : "memory"
    );

    while (1);
}

void parse_mmap(multiboot_tag_mmap *mmap_tag) 
{
    uint32_t num_entries = (mmap_tag->size - sizeof(multiboot_tag_mmap)) 
                           / mmap_tag->entry_size;

    for (uint32_t i = 0; i < num_entries; i++)
    {
        multiboot_mmap_entry *entry = &mmap_tag->entries[i];

        // Print or process the entry
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) 
        {
            MemoryRegion * region = pvPortMalloc(sizeof(MemoryRegion));
            region->addr = entry->addr;
            region->size = entry->len;
            region->type = entry->type;


            printf("addr: %x size: %x\n", region->addr, region->size);

            push_back(&memoryRegions, region);

        } 
        else 
        {
            MemoryRegion * region = pvPortMalloc(sizeof(MemoryRegion));

            region->addr = entry->addr;
            region->size = entry->len;
            region->type = entry->type;

            push_back(&memoryRegions, region);
        }
    }
}