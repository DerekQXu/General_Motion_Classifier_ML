#ifndef QUEUE_H
#define QUEUE_H
#include <stdlib.h>

//TODO: check if this is legal:
/* int* k;
 * k = malloc(...);
 * *k = 5;
 * k = k + 1;
 * *k = 10;
 * free(k);
 */
//Array implementation of Queue Data-Structure
struct Queue
{
	int front, rear, size;
	unsigned capacity;
	float* array;
};
//initiates the Queue
struct Queue *createQueue(unsigned capacity)
{
	struct Queue *queue = (struct Queue*)malloc(sizeof(struct Queue));
	queue->capacity = capacity;
	queue->front = queue->size = 0;
	queue->rear = capacity - 1;
	queue->array = (float*) malloc(queue->capacity * sizeof(float));
	return queue;
}
//removes elementes from the queue
float deQueue(struct Queue *queue)
{
	if (queue->size == 0)
		return 0;
	float item = queue->array[queue->front];
	queue->front = (queue->front + 1)%queue->capacity;
	queue->size = queue->size - 1;
	return item;
}
//add elements to the queue
void enQueue(struct Queue *queue, float item)
{
	if (queue->size == queue->capacity)
		deQueue(queue);
	queue->rear = (queue->rear + 1) % queue->capacity;
	queue->array[queue->rear] = item;
	queue->size = queue->size + 1;
}
//less safe but faster implementation of deQueue->enQueue
void denQueue(struct Queue *queue, float item)
{
	queue->front = (queue->front + 1)%queue->capacity;
	queue->rear = (queue->rear + 1) % queue->capacity;
	queue->array[queue->rear] = item;
}
//return the elt'th last stored value
float getElt(struct Queue *queue, int elt)
{
	// If queue is empty, return NULL.
	return queue->array[(queue->front + elt)%queue->capacity];
}
float clear(struct Queue *queue){
	free(queue->array); 
	free(queue);
}

#endif
