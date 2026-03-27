#include <stdint.h>
#include <stddef.h>
#include "print.h"
#include "x86_64/memory.h"

#define VIRT_BASE 0xffffffff80000000
// Define the Node structure
typedef struct MemoryRegion {
    uint64_t size;
    uint64_t addr;
    uint32_t type;
    struct MemoryRegion* next;
} MemoryRegion;

// Function to create a new node 
static MemoryRegion * createNode(uint64_t size, uint64_t addr, uint32_t type) 
{
    MemoryRegion* newNode = (MemoryRegion*) pvPortMalloc(sizeof(MemoryRegion));

    newNode->size = size;
    newNode->addr = addr;
    newNode->type = type;
    newNode->next = NULL;

    return newNode;
}

// Function to insert a new element at the beginning of the singly linked list
void insertAtFirst(struct MemoryRegion** head, uint64_t size, uint64_t addr, uint32_t type) {
    MemoryRegion* newNode = createNode(size, addr, type);
    newNode->next = *head;
    *head = newNode;
}



void push_back(MemoryRegion ** head, MemoryRegion * element) {
    if (*head == NULL) {
        *head = element;
        return;
    }

    MemoryRegion* temp = *head;
    
    while (temp->next != NULL) 
    {
        temp = temp->next;
    }

    temp->next = element;
}

// Function to insert a new element at the end of the singly linked list
void insertAtEnd(MemoryRegion** head, uint64_t size, uint64_t addr, uint32_t type) {
       MemoryRegion* newNode = createNode(size, addr, type);
    if (*head == NULL) {
        *head = newNode;
        return;
    }
    MemoryRegion* temp = *head;
    while (temp->next != NULL) {
        temp = temp->next;
    }
    temp->next = newNode;
}

// Function to insert a new element at a specific position in the singly linked list
void insertAtPosition(MemoryRegion** head, uint64_t size, uint64_t addr, uint32_t type, int position) {
     MemoryRegion* newNode = createNode(size, addr, type);
    if (position == 0) {
        insertAtFirst(head, size, addr, type);
        return;
    }
    MemoryRegion* temp = *head;
    for (int i = 0; temp != NULL && i < position - 1; i++) {
        temp = temp->next;
    }

    if (temp == NULL) {
        vPortFree(newNode);
        return;
    }
    newNode->next = temp->next;
    temp->next = newNode;
}

// Function to delete the first node of the singly linked list
void deleteFromFirst(MemoryRegion** head) {
    if (*head == NULL) 
    {
        return;
    }

    MemoryRegion* temp = *head;
    
    *head = temp->next;
    
    vPortFree(temp);
}

// Function to delete the last node of the singly linked list
void deleteFromEnd(MemoryRegion** head) {
    if (*head == NULL) {
        return;
    }
    MemoryRegion* temp = *head;
    if (temp->next == NULL) 
    {
        vPortFree(temp);
        *head = NULL;
        return;
    }
    while (temp->next->next != NULL) {
        temp = temp->next;
    }
    vPortFree(temp->next);
    temp->next = NULL;
}

// Function to delete a node at a specific position in the singly linked list
void deleteAtPosition(MemoryRegion** head, int position) {
    if (*head == NULL) {;
        return;
    }
    MemoryRegion* temp = *head;
    if (position == 0) {
        deleteFromFirst(head);
        return;
    }
    for (int i = 0; temp != NULL && i < position - 1; i++) {
        temp = temp->next;
    }
    if (temp == NULL || temp->next == NULL) {
        return;
    }
    MemoryRegion* next = temp->next->next;
    vPortFree(temp->next);
    temp->next = next;
}




void printNodes(MemoryRegion * head)
{
    MemoryRegion * current = head;

    while (current != NULL)
    {
        print_str("\n element:\n");
        print_str("\ntype: ");
        print_uint64_dec(current->type);
        print_str("\nsize: ");
        print_uint64_dec(current->size);
        print_str("\naddr: ");
        print_uint64_hex( (uint64_t) current->addr + VIRT_BASE);
        current = current->next;
    }
}