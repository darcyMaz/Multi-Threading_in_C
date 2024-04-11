#include"sut.c"

void wait_func_1(int n)
{
	struct timespec req_ = {1, 10000000};
	
	for (int i=0;i<n;i++)
	{
		printf("Wait: %d\n",i);
		nanosleep(&req_, &rem);
	}
}

void wait_func(int n)
{
	struct timespec req_ = {1, 10000000};
	
	for (int i=0;i<n;i++)
	{
		printf("Wait: %d\n",i);
		if (i == 2) sut_yield();
		nanosleep(&req_, &rem);
	}
}

void test_func_2()
{
	printf("\t\tRuntime created thread!\n");
	sut_exit();
}

void test_shutdown()
{
	sut_shutdown();
	sut_exit();
}


void test_func()
{
	printf("Threadid: %d\n", current_thread.threadid);
	if (current_thread.threadid == 2)
	{
		sut_create( test_func_2 );
	}
	wait_func(2);
	
	sut_exit();	
}

void test_func_1()
{
	printf("Threadid: %d\n", current_thread.threadid);
	wait_func(4);
	
	sut_exit();
}

void test_open()
{
	printf("Threadid: %d - open\n", current_thread.threadid);
	
	wait_func(1);
	
	
	//printf("Before sut_open() call.\n");
	int fd = sut_open("test_open.txt");
	printf("\tFile descriptor: %d.\n", fd);
	
	sut_exit();
}

void test_close()
{

	printf("Threadid: %d - close\n", current_thread.threadid);
	wait_func(1);
	
	sut_close(3);
	
	
	
	sut_exit();
}

void test_read()
{
	printf("Threadid: %d - read\n", current_thread.threadid);
	int size = 100;
	char* buf = calloc(size, sizeof(char));
	sut_read(3, buf, size);
	
	printf("buf: %s\n", buf);
	
	sut_exit();
}

void test_write()
{
	printf("Threadid: %d - write\n", current_thread.threadid);
	int size = 10;
	char* buf = calloc(size, sizeof(char));
	buf = "bufbufbuf";
	sut_write(3, buf, size);
	
	//printf("buf: %s\n", buf);
	
	sut_exit();
}

int main()
{
	sut_init();

	//printf("After sut_init().\n");
	
	//wait_func(3);
	
	//sut_create( test_open );
	
	
	// wait_func_1(5);
	//printf("Split.\n");
	sut_create( test_func );
	sut_create( test_func );
	sut_create( test_func );
	
	/*
	sut_create( test_read );
	sut_create( test_write );
	sut_create( test_close );
	sut_create( test_func_1 );
	*/
	
	
	
	sut_create( test_shutdown );
	
	
	pthread_join(C_EXEC, NULL);
	pthread_join(I_EXEC, NULL);
	
	return 0;
}

