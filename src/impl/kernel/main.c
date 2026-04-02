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
#include "x86_64/util.h"
#include "x86_64/scheduler.h"
#include "x86_64/elf.h"

extern char __kernel_heap_start;
extern char __kernel_heap_end;
extern uint8_t stack_top[];
extern uint8_t stack_bottom[];
uintptr_t kernel_start = (uintptr_t)&__kernel_heap_start;
uintptr_t kernel_end   = (uintptr_t)&__kernel_heap_end;
extern uint8_t gdt64[];
extern uint8_t udata_selector[];
extern uint8_t ucode_selector[];

// Create an actual instance of this struct in RAM
cpu_local_data single_cpu;


void write_gs_base(uint64_t address) {

    // Split 64-bit address into two 32-bit halves

    uint32_t low = (uint32_t)(address & 0xFFFFFFFF);

    uint32_t high = (uint32_t)(address >> 32);



    asm volatile (

        "wrmsr"

        : 

        : "c" (0xC0000101), // ECX: The MSR address for GS_BASE

          "a" (low),        // EAX: Lower 32 bits

          "d" (high)        // EDX: Higher 32 bits

    );

}



void init_system() {
    // Fill the struct with some test values
    single_cpu.kernel_stack = 0xAAAA5555; 
    single_cpu.user_stack   = USER_STACK_TOP;

    // Tell the CPU: GS Base = the memory address of my_cpu_data
    write_gs_base((uint64_t)&single_cpu);
}

uint64_t read_kernel_stack() {
    uint64_t result;
    asm volatile (
        "movq %%gs:16, %0"  // Read 8 bytes starting at GS_BASE + 8
        : "=r" (result)    // Output to the 'result' variable
    );
    return result;
}



void kernel_process(void)
{
    printf("Enintering kernel process\n");
    while (1)
    {

        //  printf("hello from kys\n");

        //  for(uint64_t i = 0; i < 1000000; i++);
        // asm volatile("hlt");
    };
}

extern void syscall_stub(void);

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
    // parse_elf("ELF");
    // init_system();


    
    

     
    // uint64_t val = read_kernel_stack();
    
    // printf("%x\n", val);


    // syscall_stub();

    // val = read_kernel_stack();
    
    // printf("%x\n", val);
    //  start_user_process("USER    ELF");
    
    // printf("idk: %x\n", read_gs_offset(16));

    // init_syscall_interface();
    // ;
    start_user_process("USER    ELF");
    // Thread * thread = create_userthread(test2,0x1000, 0xDEEDBEEF);
    create_thread(kernel_process, 0x1000, 0xABCDEFFF);
    start_scheduler();

    while (1)
    {
        asm volatile("hlt");
    };

}