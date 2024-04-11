#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>                                                                              
#include "YAUThreads.h"
#include "queue.h"

// This file was not written by me (Darcy Mazloum) but rather given as part of a class assignment.
// If you are the owner and would like this to be taken down simlpy email me: darcy.mazloum@gmail.com


threaddesc threadarr[MAX_THREADS];
int numthreads, curthread;
ucontext_t parent;

struct queue ready;

// Init just initializes this.
void initYAUThreads()
{
	ready = queue_create();
	queue_init(&ready);

	numthreads = 0;
	curthread = 0;
}

// So YAUSpawn creates a thread.
int YAUSpawn( void (threadfunc)() )
{
	threaddesc *tdescptr;

	if (numthreads >= MAX_THREADS)
	{
		printf("FATAL: Maximum thread limit reached... creation failed! \n");
		return -1;
	}

	tdescptr = &(threadarr[numthreads]);
	getcontext(&(tdescptr->threadcontext));
	tdescptr->threadid = numthreads;
	
	// Ok now I'm curious if char * is a problem.
	tdescptr->threadstack = (char *)malloc(THREAD_STACK_SIZE);
	
	tdescptr->threadcontext.uc_stack.ss_sp = tdescptr->threadstack;
	tdescptr->threadcontext.uc_stack.ss_size = THREAD_STACK_SIZE;
	tdescptr->threadcontext.uc_link = 0;
	tdescptr->threadcontext.uc_stack.ss_flags = 0;
	tdescptr->threadfunc = threadfunc;

	makecontext(&(tdescptr->threadcontext), threadfunc, 1, tdescptr);

	numthreads++;

	return 0;
}


void handle_timerexpiry() 
{
	printf("handle_timeexpirey()\n");

        struct sigaction handler;
	int nxtthread, curthreadsave;
	
        handler.sa_handler = handle_timerexpiry;
        sigaction(SIGALRM, &handler, NULL);
        alarm(RR_QUANTUM);

	nxtthread = (curthread +1) % numthreads;

	printf("\tcurthread: %d. nxtthread: %d. \n\tthreadarr[curthread].threadid: %d. numthreads: %d.\n", curthread, nxtthread, threadarr[curthread].threadid, numthreads);

	curthreadsave = curthread;
	curthread = nxtthread;
	
	swapcontext(&(threadarr[curthreadsave].threadcontext),
		    &(threadarr[nxtthread].threadcontext));
}
 

void startYAUThreads(int sched)
{
	struct sigaction handler;

	if (sched == RR && numthreads > 0)
	{
		handler.sa_handler = handle_timerexpiry;
		sigaction(SIGALRM, &handler, NULL);
		alarm(RR_QUANTUM);

		swapcontext(&parent, &(threadarr[curthread].threadcontext));
	}	
}


int getYAUThreadid(threaddesc *th)
{
	return th->threadid;
}

	
