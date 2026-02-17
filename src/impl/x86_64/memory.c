#include "x86_64/list.h"
#include "x86_64/memory.h"
#include "print.h"


#define HEAP_SIZE (10 * 1024 * 1024)
#define NODE_ALIGNMENT 8


void add_memory_region(uint64_t start_addr, uint64_t size);

static __attribute__((section(".heap"))) uint8_t heap[HEAP_SIZE];

static struct Node head;



void heap_init(uint64_t heap_start, uint64_t heap_size)
{

    head.next = NULL;
    head.size = 0;

    add_memory_region(heap_start, heap_size);
}

uint64_t align_up(uint64_t addr, uint64_t align) 
{
    return (addr + align - 1) & ~(align - 1);
}


void add_memory_region(uint64_t start_addr, uint64_t size)
{

    print_char('\n');
    print_uint64_hex(start_addr);

    // 0xFFFFFFFF8010C00  7    0111 & 01111111111111111000
    uint64_t aligned_addr = align_up(start_addr, NODE_ALIGNMENT);
    

    print_char('\n');
    print_uint64_hex(aligned_addr);
    if (size >= NODE_ALIGNMENT)
    {
        struct Node * current_node = (struct Node *) aligned_addr;
        current_node->size = size;
        current_node->next = NULL;
        head.next = current_node;
    }
}

struct Node * find_memory(uint64_t size)
{
    struct Node * current = &head; 

    // skip head;
    // we cant skip head cuz we need prev ref
    // current = current->next;

    while (current->next != NULL)
    {

        print_char('\n');
        print_uint64_hex(current->next->size);

        // current / head --> current.next -- > something
        if (current->next->size >= size)
        {
            // we want to remove node;
            
            struct Node * selected_region = current->next;
            current->next = current->next->next;
            // head -> .... -> current -> something

            // head -> ... -> something

            return selected_region;
        }
        else
        {
            current = current->next;
        }
    }
}


uint64_t * check_memory_region(struct Node * node, uint64_t size)
{
    if (node->size >= size)
    {
        return node;
    }
    else 
    {
        return NULL;
    }
}

uint64_t* kmalloc(uint64_t size)
{
    struct Node * node =  find_memory(size);

    if (node == NULL)
    {
        return NULL;
    }



    
    uint64_t end_addr = ((uint64_t) node) + size;
    uint64_t region_end = ((uint64_t) node) + node->size;
    uint64_t excess_size = region_end - end_addr;

    add_memory_region(end_addr, excess_size);

    return (uint64_t *) node;
}