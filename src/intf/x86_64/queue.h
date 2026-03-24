#pragma once

#include <stddef.h>
#include <stdint.h>
#include "x86_64/memory.h"
#include "x86_64/thread.h"

// Define the structure for a node of the linked list
typedef struct Node
{
    Thread * thread;
    struct Node *next;
} node;

// Define the structure for the queue
typedef struct Queue
{
    uint64_t size;
    node *front;
    node *rear;
} queue;

// Function to create a new node
node *createNode(Thread * thread);

// Function to create a new queue
queue *createQueue();

// Function to check if the queue is empty
int isEmpty(queue * q);

// Function to add an element to the queue
void enqueue(queue * q, Thread * thread);

// Function to remove an element from the queue
Thread * dequeue(queue *q);

// Function to return the front element of the queue
Thread * peek(queue *q);

// Function to print the queue
void printQueue(queue *q);