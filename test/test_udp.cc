#include <string.h>

#include <iostream>

#include "cocoflow.h"

using namespace std;

#define TEST_PORT	30917
#define TEST_TIMES	10

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
		ccf::udp u;
		char buf[65536];
		ASSERT(u.bind(ccf::ip_to_addr("0.0.0.0", TEST_PORT)) == 0);
		for (int i=0; i<TEST_TIMES; i++)
		{
			size_t len = sizeof(buf);
			ccf::udp::recv ur(u, NULL, buf, len);
			await(ur);
			cout << "recv_task recv " << len << endl;
		}
	}
};

class send_task: public ccf::user_task
{
	void run()
	{
		ccf::udp u;
		char buf[65536];
		struct sockaddr_in target = ccf::ip_to_addr("127.0.0.1", TEST_PORT);
		for (int i=0; i<TEST_TIMES; i++)
		{
			ccf::sleep s((i+1)*200);
			await(s);
			ccf::udp::send us(u, target, buf, i+1);
			await(us);
			cout << "send_task send " << i+1 << endl;
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
