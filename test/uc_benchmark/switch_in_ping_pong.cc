#include <stdio.h>
#include <time.h>
#include <ucontext.h>

#define TEST_TIMES 100000

ucontext_t ucs[3];

char sBuf1[4096];
char sBuf2[4096];

void ping()
{
	int i;
	for (i=0; i<TEST_TIMES; i++)
		swapcontext(&ucs[1], &ucs[2]);
}

void pong()
{
	int i;
	for (i=0; i<TEST_TIMES; i++)
		swapcontext(&ucs[2], &ucs[1]);
}

int main()
{
	clock_t tBgn, tEnd;
	
	tBgn = clock();
	
	getcontext(&ucs[1]);
	getcontext(&ucs[2]);
	
	ucs[1].uc_link = &ucs[0];
	ucs[1].uc_stack.ss_sp = sBuf1;
	ucs[1].uc_stack.ss_size = sizeof(sBuf1);
	ucs[2].uc_link = &ucs[0];
	ucs[2].uc_stack.ss_sp = sBuf2;
	ucs[2].uc_stack.ss_size = sizeof(sBuf2);

	makecontext(&ucs[1], ping, 0);
	makecontext(&ucs[2], pong, 0);

	swapcontext(&ucs[0], &ucs[1]);
	
	tEnd = clock();
	
	printf("%ldus\n", tEnd - tBgn);

	return 0;
}
