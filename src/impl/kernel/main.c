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

extern char __kernel_heap_start;
extern char __kernel_heap_end;
extern uint8_t stack_top[];
extern uint8_t stack_bottom[];
uintptr_t kernel_start = (uintptr_t)&__kernel_heap_start;
uintptr_t kernel_end   = (uintptr_t)&__kernel_heap_end;
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
    fs_init(multibootinfo);
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