#include <string.h>

#include <iostream>

#include "cocoflow.h"

using namespace std;

#define TEST_PORT	31005
#define TEST_TIMES	10
#define TEST_LEN	16

#define ASSERT(x) \
do { \
	if (!(x)) \
	{ \
		fprintf(stderr, "[ASSERT]: " #x " failed at " __FILE__ ":%u\n", __LINE__); \
		exit(1); \
	} \
} while(0)

class recv_task: public ccf::user_task
{
	void run()
	{
		int ret;
		ccf::tcp::listening tl(1);
		ASSERT(tl.bind(ccf::ip_to_addr("0.0.0.0", TEST_PORT)) == 0);
		ccf::tcp::connected tc;
		ccf::tcp::accept ta(ret, tl, tc);
		await(ta);
		ASSERT(ret == ccf::tcp::success);
		size_t total_len = 0;
		for (;;)
		{
			char buf[64];
			ccf::sleep s(rand()%5000);
			await(s);
			size_t len = sizeof(buf);
			ccf::tcp::recv tr(ret, tc, buf, len);
			await(tr);
			ASSERT(ret == ccf::tcp::success);
			cout << "server recv " << len << endl;
			total_len += len;
			if (total_len == TEST_TIMES * TEST_LEN)
				break;
		}
	}
};

class send_task: public ccf::user_task
{
	void run()
	{
		int ret;
		ccf::tcp::connected tc;
		ccf::tcp::connect c(ret, tc, ccf::ip_to_addr("127.0.0.1", TEST_PORT));
		await(c);
		ASSERT(ret == ccf::tcp::success);
		for (int i=0; i<TEST_TIMES; i++)
		{
			char buf[64];
			ccf::sleep s(rand()%2000);
			await(s);
			ccf::tcp::send ts(ret, tc, buf, TEST_LEN);
			await(ts);
			ASSERT(ret == ccf::tcp::success);
			cout << "client send " << TEST_LEN << endl;
		}
	}
};

class main_task: public ccf::user_task
{
	void run()
	{
		ccf::start(new recv_task());
		ccf::start(new send_task());
	}
};

int main()
{
	ccf::event_task::init(100);
	ccf::user_task::init(100);
	
	//ccf::set_debug(stderr);
	
	main_task tMain;
	ccf::cocoflow(tMain);
	return 0;
}
