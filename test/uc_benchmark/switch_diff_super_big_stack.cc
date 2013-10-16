#include <stdio.h>
#include <time.h>
#include <ucontext.h>

#define TEST_TIMES 100000

ucontext_t ucs[TEST_TIMES + 1];

char sBufs[TEST_TIMES][32768];

void ping()
{
}

int main()
{
	int i;
	clock_t tBgn, tEnd, tBrk1, tBrk2, tBrk3;
	
	tBgn = clock();
	
	for (i=0; i<TEST_TIMES; i++)
		getcontext(&ucs[i+1]);	

	tBrk1 = clock();
	
	for (i=0; i<TEST_TIMES; i++)
	{
		ucs[i+1].uc_link = &ucs[0];
		ucs[i+1].uc_stack.ss_sp = sBufs[i];
		ucs[i+1].uc_stack.ss_size = 32768; //sizeof(sBufs[i])
	}
	
	tBrk2 = clock();
	
	for (i=0; i<TEST_TIMES; i++)	
		makecontext(&ucs[i+1], ping, 0);
	
	tBrk3 = clock();
	
	for (i=0; i<TEST_TIMES; i++)
		swapcontext(&ucs[0], &ucs[i+1]);
		
	tEnd = clock();
	
	printf("  %ldus getcontext\n", tBrk1 - tBgn);
	//printf("%ldus\n", tBrk2 - tBrk1);
	printf("  %ldus makecontext\n", tBrk3 - tBrk2);
	printf("  %ldus swapcontext\n", tEnd - tBrk3);
	
	printf("%ldus\n", tEnd - tBgn);
	
	return 0;
}
