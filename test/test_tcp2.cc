#include <string.h>

#include <iostream>

#include "cocoflow.h"

using namespace std;

#define TEST_PORT	31005

#define ASSERT(x) \
do { \
	if (!(x)) \
	{ \
		fprintf(stderr, "[ASSERT]: " #x " failed at " __FILE__ ":%u\n", __LINE__); \
		abort(); \
	} \
} while(0)

class recv_task: public ccf::user_task
{
	void run()
	{
		int ret;
		char buf[64];
		ccf::tcp::listening tl(1);
		ASSERT(tl.bind(ccf::ip_to_addr("0.0.0.0", TEST_PORT)) == 0);
		ccf::tcp::connected tc;
		ccf::tcp::accept ta(ret, tl, tc);
		await(ta);
		ASSERT(ret == ccf::tcp::success);
		size_t len = sizeof(buf);
		ccf::sleep s(500);
		ccf::tcp::recv_till tr0(ret, tc, buf, len);
		ccf::any_of any(tr0, s);
		await(any);
		ASSERT(ret == ccf::tcp::unfinished);
		if (any.who_completed() == 1)
			cout << "server cancel recv " << len << endl;
		len = 20;
		ccf::tcp::recv_till tr1(ret, tc, buf, len);
		await(tr1);
		ASSERT(ret == ccf::tcp::success);
		cout << "server recv " << len << endl;
		ccf::tcp::recv_till tr2(ret, tc, buf, len, "!!", 2);
		await(tr2);
		ASSERT(ret == ccf::tcp::success);
		cout << "server recv " << len << endl;
	}
};

class send_task: public ccf::user_task
{
	void run()
	{
		int ret;
		char buf[64] = "hello world!!";
		ccf::tcp::connected tc;
		ccf::tcp::connect c(ret, tc, ccf::ip_to_addr("127.0.0.1", TEST_PORT));
		await(c);
		ASSERT(ret == ccf::tcp::success);
		ccf::tcp::send ts0(ret, tc, buf, 20);
		await(ts0);
		ASSERT(ret == ccf::tcp::success);
		cout << "client send 20" << endl;
		ccf::sleep s(1000);
		await(s);
		ccf::tcp::send ts1(ret, tc, buf, 20);
		await(ts1);
		ASSERT(ret == ccf::tcp::success);
		cout << "client send 20" << endl;
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
