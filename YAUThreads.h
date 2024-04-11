#define _XOPEN_SOURCE

#ifndef __YAUTHREAD_H__
#define __YAUTHREAD_H__

#include <ucontext.h>

// This file was not written by me (Darcy Mazloum) but rather given as part of a class assignment.
// If you are the owner and would like this to be taken down simlpy email me: darcy.mazloum@gmail.com

#define MAX_THREADS                        32
#define THREAD_STACK_SIZE                  1024*64

#define RR                                 1   // round robin
#define FCFS                               2   // first come first served

#define RR_QUANTUM                         2   // in seconds


typedef struct __threaddesc
{
	int threadid; // Is this an arbitrary system I create? Probably.
	char *threadstack;
	void *threadfunc;   // This is the same as sut_task_f type. Just cast it?
	ucontext_t threadcontext;
} threaddesc;



extern threaddesc threadarr[MAX_THREADS];
extern int numthreads, curthread;
extern ucontext_t parent;



// some function prototypes...

void initYAUThreads();
int YAUSpawn( void (threadfunc)(threaddesc *arg) );  // What does this do?
void startYAUThreads(int sched);  // 
int getYAUThreadid(threaddesc *th);  // Easy enough.


// not yet implemented..
void YAUWaitall();




#endif

