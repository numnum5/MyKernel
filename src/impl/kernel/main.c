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
#include "x86_64/mrs.h"
#include "x86_64/syscall.h"

extern char __kernel_heap_start;
extern char __kernel_heap_end;
extern uint8_t stack_top[];
extern uint8_t stack_bottom[];
uintptr_t kernel_start = (uintptr_t)&__kernel_heap_start;
uintptr_t kernel_end   = (uintptr_t)&__kernel_heap_end;
extern uint8_t gdt64[];
extern uint8_t udata_selector[];
extern uint8_t ucode_selector[];






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


static inline uint64_t read_gs_16(void)
{
    uint64_t value;
    asm volatile (
        "mov %%gs:8, %0"
        : "=r"(value)
        :
        : "memory"
    );
    return value;
}

#define MSR_GS_BASE        0xC0000101
#define MSR_KERNEL_GS_BASE 0xC0000101



static uint8_t kernel_stack[4096];
cpu_local cpu;
void init_cpu()
{
    cpu.self = (uint64_t)&cpu;
    cpu.kernel_stack = (uint64_t) kernel_stack;   // replace with real stack later
    cpu.user_stack   = USER_STACK_TOP;

    // Set GS bases
    write_msr(0xC0000101, (uint64_t)&cpu);                       // user GS
    // write_msr(0xC0000102, (uint64_t)&cpu);   // kernel GS
}

// Create an actual instance of this struct in RAM
cpu_local_data single_cpu;


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
    init_scheduler_msr();
    // parse_elf("ELF");
    // init_system();
    init_syscalls();
    init_cpu();
    // write_msr(0xC0000101, (uint64_t)&single_cpu);
    // init_cpu_gs((uint64_t) &single_cpu);

    uint64_t data = read_gs_16();
    printf("local cpu data addr: %x\n", data);
    

     
    // uint64_t val = read_kernel_stack();
    
    // printf("%x\n", val);


    // syscall_stub();

    // val = read_kernel_stack();
    
    // printf("%x\n", val);
    //  start_user_process("USER    ELF");
    
    // printf("idk: %x\n", read_gs_offset(16));

    // init_syscall_interface();
    // ;
    //

    {
        create_thread(kernel_process, 0x1000, 0xABCDEFFF);
        start_user_process("USER    ELF");
        start_scheduler();
    }


    while (1)
    {
        asm volatile("hlt");
    };

}