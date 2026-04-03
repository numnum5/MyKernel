#include "x86_64/pmm.h"
#include "x86_64/paging.h"
#include "x86_64/scheduler.h"
#include "x86_64/thread.h"

extern FileSystem fs;

void start_user_process(const char * filename)
{
     File *f = fs_open(&fs, "USER    ELF");

    // uint8_t size = 100;
    char data[f->size + 1];

    uint32_t bytes_read = fs_read(&fs, f, data, f->size + 1);

    elf_ehdr_t *elf = (elf_ehdr_t*)data;

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

    elf_phdr_t *ph = (elf_phdr_t*)(data + elf->e_phoff);

    for (int i = 0; i < elf->e_phnum; i++)
    {
        if (ph[i].p_type != 1) // PT_LOAD 
            continue;
            
        uint64_t data_pages = (ph[i].p_memsz + PAGE_SIZE - 1) / PAGE_SIZE;
        uint64_t virtual_address = ph[i].p_vaddr;
        uint64_t * region_phys = pmm_alloc(data_pages);
        uint64_t * virtual_access = (uint64_t *)((uint64_t) region_phys[0] + VIRT_BASE);


        // printf("phys: %x, virt: %x\n", region_phys[0], virtual_access);
        memset(virtual_access, 0, ph[i].p_memsz);
        map_pages(virtual_address, region_phys, data_pages, PTE_P | PTE_W | PTE_U);
        memcpy(virtual_address, data + ph[i].p_offset, ph[i].p_filesz);
        free(region_phys);
    }
    uint64_t entry = elf->e_entry;

    printf("entry: %x\n", entry);
    uint64_t pages = (0x200000 + PAGE_SIZE - 1) / 0x1000;
    uint64_t virtual_address = USER_STACK_START;
    
    uint64_t * frames = pmm_alloc(pages);
    // memset(frames[0] + VIRT_BASE, 0, 0x200000);

    printf("frame 0: %x\n", frames[0]);
    printf("frame 511: %x\n", frames[pages - 1]);
    map_pages(virtual_address, frames, pages, PAGE_PRESENT | PTE_W | PTE_U);

    uint8_t * addr = (uint8_t *)(virtual_address + ((pages - 1) * PAGE_SIZE));

    printf("the last frame addr: %x\n", addr);
    printf("user stack top: %x\n", USER_STACK_TOP);
    // vmm_translate(addr);
    // vmm_translate(USER_STACK_TOP);
    // vmm_translate(USER_STACK_TOP );
    // vmm_translate(virtual_address);

    free(frames);

    Thread * user_thread = malloc(sizeof(Thread));

    user_thread->state.frame.rip = (uint64_t) entry;
    user_thread->state.frame.cs = 0x1B;
    user_thread->state.frame.rflags = 0x202;
    user_thread->state.frame.rsp = USER_STACK_TOP;
    user_thread->state.frame.ss = 0x23;
    user_thread->pid = 0xDEADBEEF;
    user_thread->priority = 1;
    user_thread->thread_type = USER;
    
    create_thread_ring(user_thread);

    // uint64_t *frame = (uint64_t *)(USER_STACK_TOP - 5 * 8);
    // frame[0] = (uint64_t) entry;    
    // frame[1] = 0x1B;               
    // frame[2] = 0x202;              
    // frame[3] = USER_STACK_TOP;  
    // frame[4] = 0x23; 

    // Thread * thread 

    // create_thread_ring();

    // asm volatile (
    //     "mov %0, %%rsp\n"
    //     "iretq\n"
    //     :
    //     : "r"(frame)
    //     : "memory"
    // );

    printf("going to while loop no print should occur beyond this point\n");
}