#include <stdio.h>
#include "Queue.h"

int i;
struct QNode *mz;
FILE *output;
int main()
{
	struct Queue *q = createQueue(5);
	enQueue(q, 10);
	enQueue(q, 20);
	//deQueue(q);
	//deQueue(q);
	enQueue(q, 30);
	enQueue(q, 40);
	enQueue(q, 50);
	enQueue(q, 20);
	denQueue(q, 1);
	denQueue(q, 1);
	denQueue(q, 1);
	denQueue(q, 1);
	denQueue(q, 0);
	denQueue(q, 5);
	denQueue(q, 6);
	denQueue(q, 7);
	int i;
	for (i = 0; i < 5; ++i){
		printf("%f\n",getElt(q,i));
	}
	clear(q);
	return 0;
}

