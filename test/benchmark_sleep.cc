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
		abort(); \
	} \
} while(0)

static clock_t time_bgn, time_cut, time_end;

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
	time_bgn = clock();
	
	ccf::event_task::init(1);
	test_task::init(TEST_NUM + 1);
	main_task tMain;
	
	time_cut = clock();
	
	ccf::cocoflow(tMain);
	
	time_end = clock();
	
	cout << "Init: " << (time_cut - time_bgn) << endl;
	cout << "Proc: " << (time_end - time_cut) << endl;
	cout << "Total: " << (time_end - time_bgn) << endl;
	
	return 0;
}
