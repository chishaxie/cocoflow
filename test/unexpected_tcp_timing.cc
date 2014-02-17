#include <iostream>

#include "cocoflow.h"

using namespace std;

#define TEST_PORT	31005
#define TEST_TIMES	10000
#define TEST_SIZE	800

#define ASSERT(x) \
do { \
	if (!(x)) \
	{ \
		fprintf(stderr, "[ASSERT]: " #x " failed at " __FILE__ ":%u\n", __LINE__); \
		abort(); \
	} \
} while(0)

typedef ccf::task<15> test_task;

static int recv_bytes = 0, send_bytes = 0;

class recv_task: public test_task
{
	void run()
	{
		int ret;
		ccf::tcp::listening tl(1);
		ccf::tcp::connected tc;
		ASSERT(tl.bind(ccf::ip_to_addr("0.0.0.0", TEST_PORT)) == 0);
		ccf::tcp::accept ta(ret, tl, tc);
		await(ta);
		ASSERT(ret == ccf::tcp::success);
		for (;;)
		{
			char buf[8192];
			size_t len = sizeof(buf);
			ccf::tcp::recv tr(ret, tc, buf, len);
			await(tr);
			ASSERT(ret == ccf::tcp::success);
			recv_bytes += len;
			if (recv_bytes > send_bytes)
				cout << "recv(" << recv_bytes << ") > send(" << send_bytes << ")" << endl;
			if (recv_bytes == TEST_TIMES * TEST_SIZE)
				exit(0);
		}
	}
};

class send_task: public test_task
{
	static ccf::tcp::connected tc;
	void run()
	{
		int ret;
		char buf[TEST_SIZE];
		ccf::tcp::send ts(ret, send_task::tc, buf, sizeof(buf));
		await(ts);
		ASSERT(ret == ccf::tcp::success);
		send_bytes += sizeof(buf);
	}
public:
	static void init()
	{
		int ret;
		ccf::tcp::connect c(ret, send_task::tc, ccf::ip_to_addr("127.0.0.1", TEST_PORT));
		await(c);
		ASSERT(ret == ccf::tcp::success);
	}
};

ccf::tcp::connected send_task::tc;

class main_task: public test_task
{
	void run()
	{
		ccf::start(new recv_task());
		send_task::init();
		for (int i=0; i<TEST_TIMES; i++)
		{
			send_task* st = new send_task();
			ASSERT(st->status() == ccf::ready);
			ccf::start(st);
		}
	}
};

int main()
{
	ccf::event_task::init(100);
	test_task::init(11000);
	main_task tMain;
	
	ccf::cocoflow(tMain);
	
	return 0;
}
