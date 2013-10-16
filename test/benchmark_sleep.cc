#include <string.h>
#include <time.h>

#include <iostream>

#include "cocoflow.h"

using namespace std;

#define TEST_TIMES	1000
#define TEST_NUM	1000

#define ASSERT(x) \
do { \
	if (!(x)) \
	{ \
		fprintf(stderr, "[ASSERT]: " #x " failed at " __FILE__ ":%u\n", __LINE__); \
		exit(1); \
	} \
} while(0)

static clock_t bgn, cut, end;

typedef ccf::task<15> test_task;

class sleep_task: public test_task
{
	void run()
	{
		for (int i=0; i<TEST_TIMES; i++)
		{
			ccf::sleep s(0);
			await(s);
		}
	}
};

class main_task: public test_task
{
	void run()
	{
		for (int i=0; i<TEST_NUM; i++)
		{
			sleep_task* s = new sleep_task();
			ASSERT(s->status() == ccf::ready);
			ccf::start(s);
		}
	}
};

int main()
{	
	bgn = clock();
	
	ccf::event_task::init(100);
	test_task::init(1100);
	main_task tMain;
	
	cut = clock();
	
	ccf::cocoflow(tMain);
	
	end = clock();
	
	cout << "Init: " << (cut - bgn) << endl;
	cout << "Proc: " << (end - cut) << endl;
	cout << "Total: " << (end - bgn) << endl;
	
	return 0;
}
