// Standard Implementation of Queue with LL
// added denQueue function

#ifndef QUEUE_H
#define QUEUE_H
#include <stdlib.h>

struct QNode
{
	float key;
	struct QNode *next;
};
struct QNode* newNode(float k)
{
	struct QNode *temp = (struct QNode*)malloc(sizeof(struct QNode));
	temp->key = k;
	temp->next = NULL;
	return temp; 
}

struct Queue
{
	struct QNode *front, *rear;
};
//initiates the Queue
struct Queue *createQueue()
{
	struct Queue *q = (struct Queue*)malloc(sizeof(struct Queue));
	q->front = q->rear = NULL;
	return q;
}
//add elements to the queue
void enQueue(struct Queue *q, float k)
{
	struct QNode *temp = newNode(k);
	// If queue is empty, then new node is front and rear both
	if (q->rear == NULL)
	{
		q->front = q->rear = temp;
		return;
	}

	q->rear->next = temp;
	q->rear = temp;
}
//removes elementes from the queue
int deQueue(struct Queue *q)
{
	// If queue is empty, return NULL.
	if (q->front == NULL){ return 0; }

	struct QNode *temp = q->front;
	q->front = q->front->next;
	free(temp);

	// If front becomes NULL, then change rear also as NULL
	if (q->front == NULL){ q->rear = NULL; }
	return 1;
}
//adds and removes an element from the queue
void denQueue(struct Queue *q, float k)
{
	struct QNode *temp = newNode(k);
	q->rear->next = temp;
	q->rear = temp;
	temp = q->front;
	q->front = q->front->next;
	free(temp);
}
//return the elt'th last stored value
float getElt(struct Queue *q, int elt)
{
	// If queue is empty, return NULL.
	if (q->front == NULL){ return -1; }

	struct QNode *temp = q->front;

	int i;
	for (i = 0; i < elt; ++i){
		temp = temp->next;
		if (temp == NULL){ return -1; }
	}
	return temp->key;
}

#endif
