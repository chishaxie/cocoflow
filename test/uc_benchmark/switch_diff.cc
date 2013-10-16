#include <stdio.h>
#include <time.h>
#include <ucontext.h>

#define TEST_TIMES 100000

ucontext_t ucs[2];

char sBufs[TEST_TIMES][2048];

void ping()
{
}

int main()
{
	int i;
	clock_t tBgn, tEnd;
	
	tBgn = clock();
	
	for (i=0; i<TEST_TIMES; i++)
	{
		getcontext(&ucs[1]);
		
		ucs[1].uc_link = &ucs[0];
		ucs[1].uc_stack.ss_sp = sBufs[i];
		ucs[1].uc_stack.ss_size = 2048; //sizeof(sBufs[i])
		
		makecontext(&ucs[1], ping, 0);
		
		swapcontext(&ucs[0], &ucs[1]);
	}
	
	tEnd = clock();
	
	printf("%ldus\n", tEnd - tBgn);

	return 0;
}
