#include <stddef.h>
#include <stdint.h>
#include "x86_64/memory.h"
#include "x86_64/queue.h"
#include "x86_64/scheduler.h"


// Function to create a new node
node *createNode(Thread * thread)
{
    node *newNode = (node *)pvPortMalloc(sizeof(node));
    if (newNode == NULL) return NULL;
    newNode->thread = thread;
    newNode->next = NULL;
    return newNode;
}

// Function to create a new queue
queue *createQueue()
{
    queue *newQueue = (queue *)pvPortMalloc(sizeof(queue));
    newQueue->front = newQueue->rear = NULL;
    newQueue->size = 0;
    return newQueue;
}

// Function to check if the queue is empty
int isEmpty(queue * q)
{
    return q->front == NULL;
}

void enqueue(queue * q, Thread * thread)
{
    // Create a new node with the given data
    node *newNode = createNode(thread);
    if (! newNode)
    {
        return;
    }
    q->size++;

    if (q->rear == NULL)
    {
        q->front = q->rear = newNode;
        return;
    }

    q->rear->next = newNode;
    q->rear = newNode;
}

// Function to remove an element from the queue
Thread * dequeue(queue *q)
{
    if (isEmpty(q))
    {
        return NULL;
    }
    node *temp = q->front;
    q->front = q->front->next;
    if (q->front == NULL)
    {
        q->rear = NULL;
    }
    Thread * thread = temp->thread;

    q->size--;

    vPortFree(temp);

    return thread;
}

Thread * peek(queue *q)
{
    if (isEmpty(q))
        return NULL;
    return q->front->thread;
}

// Function to print the queue
void printQueue(queue *q)
{
    node *temp = q->front;
    while (temp != NULL)
    {
        temp = temp->next;
    }
}
