#include <stddef.h>
#include <stdint.h>
#include "x86_64/memory.h"
#include "x86_64/queue.h"
#include "x86_64/scheduler.h"

// Function to create a new node
node *createNode(Thread * thread)
{
    // Allocate memory for a new node
    node *newNode = (node *)pvPortMalloc(sizeof(node));
    // Check if memory allocation was successful
    if (newNode == NULL) return NULL;
    // Initialize the node's data and next pointer
    newNode->thread = thread;
    newNode->next = NULL;
    return newNode;
}

// Function to create a new queue
queue *createQueue()
{
    // Allocate memory for a new queue
    queue *newQueue = (queue *)pvPortMalloc(sizeof(queue));
    // Initialize the front and rear pointers of the queue
    newQueue->front = newQueue->rear = NULL;
    newQueue->size = 0;
    return newQueue;
}

// Function to check if the queue is empty
int isEmpty(queue * q)
{
    // Check if the front pointer is NULL
    return q->front == NULL;
}

// Function to add an element to the queue
void enqueue(queue * q, Thread * thread)
{
    // Create a new node with the given data
    node *newNode = createNode(thread);
    // Check if memory allocation for the new node was
    // successful
    if (! newNode)
    {
        return;
    }
    // If the queue is empty, set the front and rear
    // pointers to the new node

    q->size++;

    if (q->rear == NULL)
    {
        q->front = q->rear = newNode;
        return;
    }
    // Add the new node at the end of the queue and update
    // the rear pointer

    q->rear->next = newNode;
    q->rear = newNode;
}

// Function to remove an element from the queue
Thread * dequeue(queue *q)
{
    // Check if the queue is empty
    if (isEmpty(q))
    {
        return NULL;
    }
    // Store the front node and update the front pointer
    node *temp = q->front;
    q->front = q->front->next;
    // If the queue becomes empty, update the rear pointer
    if (q->front == NULL)
    {
        q->rear = NULL;
    }
    // Store the data of the front node and free its memory
    Thread * thread = temp->thread;

    q->size--;

    vPortFree(temp);

    return thread;
}

// Function to return the front element of the queue
Thread * peek(queue *q)
{
    // Check if the queue is empty
    if (isEmpty(q))
        return NULL;
    // Return the data of the front node
    return q->front->thread;
}

// Function to print the queue
void printQueue(queue *q)
{
    // Traverse the queue and print each element
    node *temp = q->front;
    while (temp != NULL)
    {
        // printf("%d -> ", temp->thread);
        temp = temp->next;
    }
}
