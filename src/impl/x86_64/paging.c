#include "x86_64/paging.h"
#include "x86_64/list.h"
#include "x86_64/memory.h"
#include "x86_64/pmm.h"
#include "print.h"

static uint64_t get_l4_page_table(void)
{
    uint64_t cr3;
    asm volatile ("mov %%cr3, %0" : "=r"(cr3));
    return cr3 & 0x000FFFFFFFFFF000ULL;
}

void map_pages(uint64_t virt_addr, uint64_t * phys_addr, uint64_t count, uint64_t flags)
{   
    for (uint64_t  i = 0; i < count; i++)
    {
        uint64_t current_virt_addr = i * PAGE_SIZE + virt_addr;
        map_page(current_virt_addr, phys_addr[i], flags);
        // printf("current: %x, physical: %x\n", current_virt_addr, phys_addr[i]);
    }
}

void map_page(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags)
{
    uint64_t pml4_idx = (virt_addr >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_idx   = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_idx   = (virt_addr >> 12) & 0x1FF;
    uint64_t *pml4 = (uint64_t *)(get_l4_page_table() + VIRT_BASE);


    if (!(pml4[pml4_idx] & PAGE_PRESENT)) {
        uint64_t frame = pmm_alloc_frame();
        memset((void*)(frame + VIRT_BASE), 0, 4096);
        pml4[pml4_idx] = frame | PAGE_PRESENT | PTE_W | PTE_U;
    }

    uint64_t *pdpt = (uint64_t *)((pml4[pml4_idx] & ADDRESS_MASK) + VIRT_BASE);

    if (!(pdpt[pdpt_idx] & PAGE_PRESENT)) {
        uint64_t frame = pmm_alloc_frame();
        memset((void*)(frame + VIRT_BASE), 0, 4096);
        pdpt[pdpt_idx] = frame | PAGE_PRESENT | PTE_W | PTE_U;
    }

    uint64_t *pd = (uint64_t *)((pdpt[pdpt_idx] & ADDRESS_MASK) + VIRT_BASE);

    if (!(pd[pd_idx] & PAGE_PRESENT)) 
    {
        uint64_t frame = pmm_alloc_frame();
        memset((void*)(frame + VIRT_BASE), 0, 4096);
        pd[pd_idx] = frame | PAGE_PRESENT | PTE_W | PTE_U;
    }

    uint64_t *pt = (uint64_t *)((pd[pd_idx] & ADDRESS_MASK) + VIRT_BASE);

    pt[pt_idx] = (phys_addr) | PAGE_PRESENT | PTE_W | PTE_U;

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

    uint64_t * pml4 = (uint64_t *)(get_l4_page_table() + VIRT_BASE);
    
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
