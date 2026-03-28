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
#include "x86_64/idt.h"
#include "x86_64/pmm.h"
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

void test_user_function(void)
{
    // print_str("Hello from task2!\n");
    while (1){
        // for(volatile int i = 0; i < 100000000; i++);
    };

}

void test2(void)
{
    // print_str("Hello from task34!\n");
    while (1){
        // for(volatile int i = 0; i < 100000000; i++);
    };
}

extern uint8_t stack_top[];
extern uint8_t stack_bottom[];
extern uint8_t gdt64[];
extern uint8_t udata_selector[];
extern uint8_t ucode_selector[];

void kernel_main(uint32_t magic, void * multibootinfo) 
{
    print_clear();
    print_set_color(PRINT_COLOR_YELLOW, PRINT_COLOR_BLACK);
    print_str("Welcome to our OS kernel!\n");

    tss_init();
    idt_init();
    pit_init();
    pmm_init(multibootinfo);
    scheduler_init();
    // init_syscall_interface();
    // ;
    // scheduler_init();
    // Thread * thread = create_userthread(test2,0x1000, 0xDEEDBEEF);
    // create_thread(test,0x1000, 0xABCDEFFF);
    // start_scheduler();


    // Initialise physical memory mananger
   


    uint64_t code_phys  = pmm_alloc_frame();
    uint64_t stack_phys = pmm_alloc_frame();

    // #define USER_CODE  0x400000
    // #define USER_STACK 0x500000
    // map_page2(USER_CODE,  code_phys,  PAGE_PRESENT | PTE_W | PTE_U);
    // map_page2(USER_STACK, stack_phys, PAGE_PRESENT | PTE_W | PTE_U);
    // vmm_translate(USER_CODE);
    // vmm_translate(USER_STACK);

    // uint8_t *code = (uint8_t*)USER_CODE;
    // uint64_t *frame = (uint64_t *)(USER_STACK + 0x1000 - 5 * 8);
    // code[0] = 0xEB; // JMP short
    // code[1] = 0xFE; // jump back 2 bytes
    // frame[0] = (uint64_t) code;    
    // frame[1] = 0x1B;               
    // frame[2] = 0x202;              
    // frame[3] = USER_STACK + 0x1000;  
    // frame[4] = 0x23;               

    while (1)
    {
        asm volatile("hlt");
    };
}