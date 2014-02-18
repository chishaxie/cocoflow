#include <string.h>
#include <time.h>

#include <iostream>

#include "cocoflow.h"

using namespace std;

#define TEST_PORT	30917
#ifdef _WIN32
#define TEST_TIMES	1000 //lite
#else
#define TEST_TIMES	10000
#endif

#define ASSERT(x) \
do { \
	if (!(x)) \
	{ \
		fprintf(stderr, "[ASSERT]: " #x " failed at " __FILE__ ":%u\n", __LINE__); \
		abort(); \
	} \
} while(0)

static void my_pkg_seq_unrecv(const void*, size_t, const ccf::uint32&)
{
	ASSERT(0);
}

static void my_pkg_seq_failed(const void*, size_t, int)
{
	ASSERT(0);
}

static clock_t time_bgn, time_cut, time_end;
static int sleep_time = 2;

typedef ccf::task<15> test_task;

class echo_task: public test_task
{
	static int times;
	static ccf::udp u;
	void run()
	{
		char buf[4096];
		struct sockaddr_in peer;
		size_t len = sizeof(buf);
		ccf::udp::recv ur(echo_task::u, buf, len);
		await(ur);
		ASSERT(ur.peer_type() == AF_INET);
		peer = ur.peer_addr_ipv4();
		if (++echo_task::times < TEST_TIMES)
		{
			echo_task* echo = new echo_task();
			ASSERT(echo->status() == ccf::ready);
			ccf::start(echo);
		}
		ccf::udp::send us(echo_task::u, peer, buf, len);
		await(us);
	}
public:
	static void init()
	{
		ASSERT(u.bind(ccf::ip_to_addr("0.0.0.0", TEST_PORT)) == 0);
	}
};

int echo_task::times = 0;
ccf::udp echo_task::u;

int get_seq_from_buf(const void* buf, size_t size, ccf::uint32* seq)
{
	if (size < sizeof(ccf::uint32))
		return -1;
	*seq = ntohl(*(ccf::uint32*)buf);
	return 0;
}

class seq_task: public test_task
{
	static int times;
	static ccf::udp u;
	static struct sockaddr_in target;
	ccf::uint32 seq;
	void run()
	{
		char buf[4096];
		ccf::uint32 *pos = (ccf::uint32 *)buf;
		*pos = htonl(this->seq);
		ccf::udp::send us(seq_task::u, seq_task::target, buf, sizeof(ccf::uint32));
		await(us);
		size_t len = sizeof(buf);
		ccf::udp::recv_by_seq_u32 ur(seq_task::u, buf, len, this->seq);
		await(ur);
		if (++seq_task::times == TEST_TIMES)
		{
			time_end = clock();
			cout << "Init: " << (time_cut - time_bgn) << endl;
			cout << "Proc: " << (time_end - time_cut) << endl;
			cout << "Total: " << (time_end - time_bgn) << endl;
			exit(0);
		}
	}
public:
	static void init()
	{
		seq_task::target = ccf::ip_to_addr("127.0.0.1", TEST_PORT);
		ASSERT(seq_task::u.bind(get_seq_from_buf, my_pkg_seq_unrecv, my_pkg_seq_failed) == 0);
	}
	seq_task(ccf::uint32 seq) : seq(seq) {}
};

int seq_task::times = 0;
ccf::udp seq_task::u;
struct sockaddr_in seq_task::target;

class main_task: public test_task
{
	void run()
	{
		echo_task::init();
		seq_task::init();
		ccf::start(new echo_task());
		for (int i=0; i<TEST_TIMES; i++)
		{
			if ((i+1)%100 == 0) //Avoid udp error
			{
				ccf::sleep s(sleep_time);
				await(s);
			}
			seq_task* seq = new seq_task(i);
			ASSERT(seq->status() == ccf::ready);
			ccf::start(seq);
		}
	}
};

int main(int argc, char* argv[])
{
	if (argc > 1)
		sleep_time = atoi(argv[1]);
	
	time_bgn = clock();
	
	ccf::event_task::init(1);
	test_task::init(TEST_TIMES + 101);
	main_task tMain;
	
	time_cut = clock();
	
	ccf::cocoflow(tMain);
	
	return 0;
}
