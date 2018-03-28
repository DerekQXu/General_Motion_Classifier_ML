#include <stdio.h>
#include "Queue.h"

int main()
{
	struct Queue *q = createQueue();
	enQueue(q, 10);
	enQueue(q, 20);
	//deQueue(q);
	//deQueue(q);
	enQueue(q, 30);
	enQueue(q, 40);
	enQueue(q, 50);
	denQueue(q, 20);
	int ret = 1;
	while (ret != 0){
		printf("%f",getElt(q, 0));
		ret = deQueue(q);	
	}
	/*if (n != NULL)
		printf("Dequeued item is %d", n->key);
	return 0;
	*/
}

