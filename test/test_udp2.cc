#include <string.h>

#include <iostream>

#include "cocoflow.h"

#include "simple_rand.h"

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

class echo_task: public ccf::user_task
{
	void run()
	{
		ccf::udp u;
		char buf[65536];
		struct sockaddr_in peer;
		ASSERT(u.bind(ccf::ip_to_addr("0.0.0.0", TEST_PORT)) == 0);
		for (int i=0; i<TEST_TIMES; i++)
		{
			size_t len = sizeof(buf);
			ccf::uint64 t = simple_rand()%2000;
			ccf::udp::recv ur(u, buf, len);
			await(ur);
			ASSERT(ur.peer_type() == AF_INET);
			peer = ur.peer_addr_ipv4();
			cout << "echo_task recv " << len << endl;
			cout << "echo_task sleep " << t << endl;
			ccf::sleep s(t);
			await(s);
			ccf::udp::send us(u, peer, buf, len);
			await(us);
			cout << "echo_task send " << len << endl;
		}
	}
};

int get_seq_from_buf(const void* buf, size_t size, const void** pos, size_t* len)
{
	if (size < sizeof(ccf::uint32))
		return -1;
	*pos = buf;
	*len = sizeof(ccf::uint32);
	return 0;
}

class seq_task: public ccf::user_task
{
	void run()
	{
		ccf::udp u;
		char buf[65536];
		struct sockaddr_in target = ccf::ip_to_addr("127.0.0.1", TEST_PORT);
		ASSERT(u.bind(get_seq_from_buf) == 0);
		ccf::uint32 *seq = (ccf::uint32 *)buf;
		for (int i=0; i<TEST_TIMES; i++)
		{
			*seq = i;
			ccf::udp::send us(u, target, buf, sizeof(ccf::uint32) + i);
			await(us);
			cout << "seq_task send " << sizeof(ccf::uint32) + i << ", seq = " << *seq << endl;
			size_t len = sizeof(buf);
			ccf::udp::recv_by_seq ur(u, buf, len, *seq);
			await(ur);
			cout << "seq_task recv " << len << ", seq = " << *seq << endl;
		}
	}
};

class main_task: public ccf::user_task
{
	void run()
	{
		ccf::start(new echo_task());
		ccf::start(new seq_task());
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
