/*

Darcy Mazloum
260987312
COMP 310 - Assignment 2
2023-11-11

An important assumption for this assignment is that a task must end by facing either a yield or exit.
This file was written by me!

*/


#include"sut.h"
#include"queue.h"
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<signal.h>
#include<stdbool.h>
#include<ucontext.h>
#include<pthread.h>
#include<time.h>
#include<fcntl.h>

#define MAX_THREADS 64
#define THREAD_STACK_SIZE 1024*64

// Global fields should be queues and two threads.
pthread_t I_EXEC;
pthread_t C_EXEC;

// Nanosleep timespecs.
struct timespec rem, req = {0, 500000000};

// Ready Queue
struct queue ready;

// Wait Queue
struct queue wait;

// TCB taken from YAUThreads.
typedef struct __threaddesc
{
	int threadid;
	char *threadstack;
	void *threadfunc;
	ucontext_t threadcontext;
} threaddesc;


threaddesc current_thread;
ucontext_t cex_context;

threaddesc current_thread_io;
ucontext_t iox_context;

threaddesc threadarr[MAX_THREADS];

// Locks
int c_lock;

bool shutdown_flag;
bool done_flag;

void non_func() {}
void init_threadarr()
{
	threaddesc *tdescptr;
	for (int ta_i = 0;ta_i<MAX_THREADS;ta_i++)
	{
		tdescptr = &(threadarr[ta_i]);
		
		getcontext(&(tdescptr->threadcontext));
		tdescptr->threadid = -1;
		
		tdescptr->threadstack = (char *)malloc(THREAD_STACK_SIZE);
		tdescptr->threadcontext.uc_stack.ss_sp = tdescptr->threadstack;
		tdescptr->threadcontext.uc_stack.ss_size = THREAD_STACK_SIZE;
		tdescptr->threadcontext.uc_link = NULL;
		tdescptr->threadcontext.uc_stack.ss_flags = 0;
		tdescptr->threadfunc = (void*)(non_func);

		makecontext(&(tdescptr->threadcontext), non_func, 1, tdescptr);
	}
}


void* i_exec(void* arg)
{
	
	struct queue_entry *popped;
	
	while (true)
	{
	
		popped = queue_pop_head(&wait);	
		
		// If the pointer is false it means the queue was empty.
		if (!popped)
		{
			
			// If the shutdown flag is 
			if (shutdown_flag && done_flag)
			{
				break;
			}
			
			// If the queue is empty but not shutdowning down then sleep and loop.
			nanosleep(&req, &rem);	
			continue;			
		}
	
		// Grab the io_td object.
		// Switch right away to its context.
		current_thread_io = (*(threaddesc*)popped->data);
		
		swapcontext(&(iox_context) , &(current_thread_io.threadcontext));
		
	}
	
	return NULL;
}

void* c_exec(void* arg)
{
	// Continuously checks the task ready queue to see if there are any threads that it can start.
	
	struct queue_entry *popped;
	
	while (true)
	{
	
		// The lock will be initialized to 0, set to 1 when a task starts, and reset to 0 when a task yields or exits.
		while (c_lock == 1)
		{
			nanosleep(&req, &rem);
		}
	
		popped = queue_pop_head(&ready);	
		
		// If the pointer is false it means the queue was empty.
		if (!popped)
		{
			
			// If the shutdown flag is false while the queue is empty it means we must shutdown the executor.
			if (shutdown_flag)
			{
				done_flag = true;
				break;
			}
			
			// If the queue is empty but not shutdowning down then sleep and loop.
			nanosleep(&req, &rem);	
			continue;			
		}
		
		
		current_thread = *(threaddesc*)popped->data;
		c_lock = 1;
		
		swapcontext(&(cex_context), &(current_thread.threadcontext));
		
		
	}
	
	return NULL;
}

/**
 *  Initalize the SUT Library.
 *  Initalize the queues and two kernal threads.
 */
void sut_init()
{
	
	init_threadarr();

	shutdown_flag = false;
	done_flag = false;
	c_lock = 0;

	ready = queue_create();
	wait = queue_create();
	queue_init(&ready);
	queue_init(&wait);

	// Initialize two queues and two threads.
	pthread_create(&I_EXEC,NULL,i_exec,NULL);
	pthread_create(&C_EXEC,NULL,c_exec,NULL);
	
	// If there is no delay. The threads are not created properly.
	usleep(1000);
}

/**
 *
 * Create a task with the given function as its main body.
 * Compiles perfectly fine. Has not been tested properly.
 *
 */
bool sut_create(sut_task_f fn)
{

	if (shutdown_flag) return false;
	
	int thread_id = -2;
	
	// Finds an available spot in the threadarr to put the id of this new thread.
	for (int thrarr_i = 0; thrarr_i<MAX_THREADS; thrarr_i++)
	{
		// When an integer in threadarr is -1, it means no thread occupies that spot.
		if (threadarr[thrarr_i].threadid == -1)
		{
			thread_id = thrarr_i;
			break;
		}
	}
	
	// There were no spots in threadarr with -1, so it must be full.
	if (thread_id == -2)
	{
		printf("FATAL: Maximum thread limit reached... creation failed! \n");
		return false;
	}
	
	
	threaddesc *tdescptr;
	
	tdescptr = &(threadarr[thread_id]);
	
	getcontext(&(tdescptr->threadcontext));
	tdescptr->threadid = thread_id;
	tdescptr->threadstack = (char *)malloc(THREAD_STACK_SIZE);
	tdescptr->threadcontext.uc_stack.ss_sp = tdescptr->threadstack;
	tdescptr->threadcontext.uc_stack.ss_size = THREAD_STACK_SIZE;
	tdescptr->threadcontext.uc_link = NULL;
	tdescptr->threadcontext.uc_stack.ss_flags = 0;
	tdescptr->threadfunc = fn;
	
	makecontext(&(tdescptr->threadcontext), fn, 1, tdescptr);
	

	// Make a node and push it onto the ready stack.
	struct queue_entry *node = queue_new_node(&threadarr[thread_id]);
    	queue_insert_tail(&ready, node);

	return true;
}

/**
 *  Allows a running task to yield execution before completion.
 */
void sut_yield()
{
	int threadid_ =  current_thread.threadid;
	struct queue_entry *node = queue_new_node(&(threadarr[threadid_]));
	queue_insert_tail(&ready, node);

	c_lock = 0;
	swapcontext(&(  (*(threaddesc*)node->data).threadcontext  ), &(cex_context) );
}


/**
 *  Immediately terminates the execution of a task. Only terminates the current
 *  task.
 */
void sut_exit()
{
	c_lock = 0;
	
	// Set the current thread's ID to -1.
	// Setting the id to -1 allows this program to know that it's place in threadarr can be filled in.
	current_thread.threadid = -1;
	
	setcontext(&(cex_context));
}

/**
 *  Opens a file specified by the name.
 *  Return a negative number if the file does not exist.
 *  Otherwise return a positive number. The location on the FDT.
 */
int sut_open(char* fname)
{
	// Add the context and call to sut_open to the wait queue.
	// Give the context back to C_Exec.
	int threadid_ =  current_thread.threadid;
	
	struct queue_entry *node = queue_new_node(&(threadarr[threadid_]));
	queue_insert_tail(&wait, node);

	c_lock = 0;
	swapcontext( &(  (*(threaddesc*)node->data).threadcontext  ), &(cex_context) );
	
	
	int toReturn = open(fname, O_RDWR);
	
	node = queue_new_node(&(threadarr[threadid_]));
	queue_insert_tail(&ready, node);

	// Now return to io_exec
	swapcontext( &(  (*(threaddesc*)node->data).threadcontext  ), &(iox_context));

	return toReturn;
}

/**
 *  Take buf and write it to the file fd. 
 *
 */
void sut_write(int fd, char* buf, int size)
{
	if (fd < 0)
	{
		fprintf(stderr, "sut_write() failure: file descriptor (fd: int) was less than zero.\n");
		return ;
	}

	// Put this context into the wait queue.
	int threadid_ =  current_thread.threadid;
	
	struct queue_entry *node = queue_new_node(&(threadarr[threadid_]));
	queue_insert_tail(&wait, node);

	c_lock = 0;
	swapcontext( &(  (*(threaddesc*)node->data).threadcontext  ), &(cex_context) );
	
	// i_exec swaps to this context and writes to the file.
	
	// Write errors are not to be considered.
	write(fd, buf, size);
	
	// Put this context back into the ready queue.
	node = queue_new_node(&(threadarr[threadid_]));
	queue_insert_tail(&ready, node);

	// Now return to io_exec. C_Exec will come back to this context later.
	swapcontext( &(  (*(threaddesc*)node->data).threadcontext  ), &(iox_context));
	
}

/**
 *  Close the file fd.
 */
void sut_close(int fd)
{
	// Put node onto wait queue
	// Give control to C_exec
	int threadid_ =  current_thread.threadid;
	
	struct queue_entry *node = queue_new_node(&(threadarr[threadid_]));
	queue_insert_tail(&wait, node);

	c_lock = 0;
	swapcontext( &(  (*(threaddesc*)node->data).threadcontext  ), &(cex_context) );
	
	
	// After swap, do close func
	// Put node onto ready queue
	close(fd);
	
	node = queue_new_node(&(threadarr[threadid_]));
	queue_insert_tail(&ready, node);
	
	swapcontext( &(  (*(threaddesc*)node->data).threadcontext  ), &(iox_context));
	
	// After swap return value of close.
	// There's no return value so that's it.
	
}

/**
 *  Read from fd and put it into buf.
 */
char* sut_read(int fd, char* buf, int size)
{

	if (fd < 0)
	{
		fprintf(stderr, "sut_read() failure: file descriptor (fd: int) was less than zero.\n");
		return buf;
	}

	// Put node onto wait queue
	// Give control to C-Exec
	int threadid_ =  current_thread.threadid;
	
	struct queue_entry *node = queue_new_node(&(threadarr[threadid_]));
	queue_insert_tail(&wait, node);

	c_lock = 0;
	swapcontext( &(  (*(threaddesc*)node->data).threadcontext  ), &(cex_context) );

	// At this point i_exec has taken control and can do the reading.
	int bytes_read = read(fd, buf, size);
	// Reading done!
	
	// Then the TCB is put back into the ready queue and the context is swapped back to io_exec.
	node = queue_new_node(&(threadarr[threadid_]));
	queue_insert_tail(&ready, node);
	
	swapcontext( &(  (*(threaddesc*)node->data).threadcontext  ), &(iox_context));	
	
	// At this point c_exec takes back control and returns our buf address :)
	return buf;
}

/**
 *  Shutdown everything gracefully. Let threads complete.
 */
void sut_shutdown()
{
	// Simply change this flag and 
	//	(1) threads can no longer be created and 
	//	(2) the executors will shutdown when the queues are empty.
	// There are two flags: shutdown_flag and done_flag.
	//	When the shutdown_flag is flipped the done_flag will
	//	also flip when both queues are empty. The logic for this
	//	can be found in the executor functions i_exec() and c_exec().
	
	shutdown_flag = true;
}


