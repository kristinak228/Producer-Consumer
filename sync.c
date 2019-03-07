/************************
CS444/544 Operating System
Week 07: Solving the producer/consumer problem

Compile Windows: gcc sync.c -pthread -DWINDOWS
Execute: ./a.out

Compile Clang: clang.exe sync.c -pthread -DWINDOWS
Execute: ./a.exe

I ended up using this option!
Compile Developer Command Prompt: cl sync.c -DWINDOWS
Execute: ./a.exe

Name: Kristina Kolibab
Clarkson University  SP2019
**************************/ 

/***** Includes *****/
#include <stdio.h>
#include <stdbool.h> /* Include true/false keywords */
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#ifdef WINDOWS
	#include <windows.h>
#endif 

/***** Defines *****/
#define PRODUCER_SLEEP_S	1 /* in seconds */
#define CONSUMER_SLEEP_S	1
#define QUEUESIZE			5
#define NUM_THREADS			10

/***** Function prototypes *****/
void *producer (void *args);
void *consumer (void *args);

/***** Queue struct *****/
typedef struct {
	int buf[QUEUESIZE];
	long head, tail;
	int full, empty;
	int buffer;
	
	/*This is my mutually exclusive lock, I will
	use this when adding/removing from the queue
	or incr/decr my buffer integer data member */
#ifdef WINDOWS
	HANDLE ghMutex;
#endif
	
} queue;

/***** Queue function prototypes  *****/
queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q, int in);
void queueDel (queue *q, int *out);

/***** main *****/
int main ()
{
	queue *fifo;
	int i; 
	int j = 0;
	unsigned int iseed = (unsigned int)time(NULL);
	srand(iseed);

#ifdef WINDOWS
	/* initialize the threads, 1 producer, 10 consumers */
	HANDLE pro, con[NUM_THREADS];
#endif

	fifo = queueInit ();
	if (fifo ==  NULL) {
		fprintf (stderr, "main: Queue Init failed.\n");
		exit (1);
	}
	
	printf("Inside Main()\n");
	
#ifdef WINDOWS /* this is a big block , but everything in it should be here */
	printf("creating producer\n");
	pro = CreateThread(NULL, 0,(LPTHREAD_START_ROUTINE) producer, fifo, 0, NULL);
	for(i=0; i<NUM_THREADS; i++)
	{
		printf("creating thread %d\n", j+1);
		j++;
	    con[i] = CreateThread(NULL, 0,(LPTHREAD_START_ROUTINE) consumer, fifo, 0, NULL);		
		Sleep((rand()%2)*1000);
	}

	/* Wait for all the threads complete */
	WaitForSingleObject(pro, INFINITE);     
	WaitForMultipleObjects(NUM_THREADS, con, TRUE, INFINITE);	
#endif

	queueDelete(fifo);

#ifdef WINDOWS
	system("pause");
#endif

	return 0;
}

/***** producer *****/
void *producer (void *q) 
{
	queue *fifo;
	int i;
	fifo = (queue *)q;
	
	for (i = 0; i < NUM_THREADS; i++)
	{
		/* Check if the buffer is full, if so wait until consumers consume */
		if (fifo->full) printf("producer must wait, buffer is full ------\n");
		while (fifo->full) {/* waiting on consumers */}

		/* Entering critical section, here my producer will produce data for
		the buffer, print out so, incr the buffer, then release the lock */
#ifdef WINDOWS
		WaitForSingleObject(fifo->ghMutex, INFINITE);
#endif
		queueAdd(fifo, i + 1);
		printf("producer added item to buffer\n");
		fifo->buffer++;
#ifdef WINDOWS
		ReleaseMutex(fifo->ghMutex);
#endif

		/* Check to make sure the empty/full data members are being updated properly */
		if (fifo->buffer <= QUEUESIZE && fifo->buffer >= 1) fifo->empty = 0;
		if (fifo->buffer == QUEUESIZE) fifo->full = 1;

#ifdef WINDOWS
		Sleep(PRODUCER_SLEEP_S * 1000);
#endif
	}

	return (NULL);
}

/***** consumer *****/
void *consumer (void *q) 
{
	queue *fifo;
	int d;
	fifo = (queue *)q;

	/* I am using a do-while loop so that any consumers that acquried the lock, only
	to see that the buffer is now empty again, can jump back and start over */
	do {
		/* Check if the buffer is empty, if so wait until producer produces */
		if (fifo->empty) /*printf("customers must wait, buffer is empty\n");*/
		while (fifo->empty) {/* waiting on producer */ }

		/* Entering critical section, here my consumers will (separately) acquire
		the lock and check again if the buffer is empty. If so, release the lock 
		and start the do-while loop over. If not, consume from the buffer, print
		out so, decr the buffer, then release the lock */
#ifdef WINDOWS
		WaitForSingleObject(fifo->ghMutex, INFINITE);
#endif
		/* Must check again that there is something to consume */
		if (fifo->empty) { 
			/* If not skip consumption, release lock, and try again */ 
#ifdef WINDOWS
			ReleaseMutex(fifo->ghMutex);
#endif
		}
		else {
			/* If so, consume from buffer, handle details, release lock */
			queueDel(fifo, &d);
			printf("consumer removed item from buffer\n");
			fifo->buffer--;
#ifdef WINDOWS
			ReleaseMutex(fifo->ghMutex);
#endif
			break; /* break out of do-while loop */
		}
	} while (true);

	/* Check to make sure the empty/full data members are being updated properly */
	if (fifo->buffer <= 4 && fifo->buffer >= 0) fifo->full = 0;
	if (fifo->buffer == 0) fifo->empty = 1;

#ifdef WINDOWS
	Sleep(CONSUMER_SLEEP_S * 1000);
#endif

	return (NULL);
}

/***** queueInit *****/
queue *queueInit (void)
{
	queue *q;
	int i;
	
	q = (queue *)malloc (sizeof (queue));
	if (q == NULL) return (NULL);

	for(i=0;i<QUEUESIZE;i++)
	{
		q->buf[i]=0;
	}
	q->empty = 1; 
	q->full = 0;
	q->head = 0;
	q->tail = 0;
	q->buffer = 0;

    /* Initialize lock here */
#ifdef WINDOWS
	q->ghMutex = CreateMutex(NULL, false, NULL);
#endif
		
	return (q);
}

/***** queueDelete *****/
void queueDelete (queue *q)
{
	/* free mutex lock here */
#ifdef WINDOWS
	CloseHandle(q->ghMutex);
#endif

	/* free memory used for queue */
	free (q);
}

/***** queueAdd *****/
void queueAdd (queue *q, int in)
{
	q->buf[q->tail] = in;
	q->tail++;
	if (q->tail == QUEUESIZE)
		q->tail = 0;
	if (q->tail == q->head)
		q->full = 1;
	else { q->full = 0; } /* ensure if not full, full is set to false */
	q->empty = 0;

	return;
}

/***** queueDel *****/
void queueDel (queue *q, int *out)
{
	*out = q->buf[q->head];
	q->buf[q->head]=0;

	q->head++;

	if (q->head == QUEUESIZE)
		q->head = 0;
	if (q->head == q->tail)
		q->empty = 1;
	else { q->empty = 0; } /* ensure if not empty, empty is set to false */
	q->full = 0;

	return;
}
