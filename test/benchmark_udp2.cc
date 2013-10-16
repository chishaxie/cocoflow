#include <string.h>
#include <time.h>

#include <iostream>

#include "cocoflow.h"

using namespace std;

#define TEST_PORT	30917
#define TEST_TIMES	10000

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

class echo_task: public test_task
{
	static int times;
	static ccf::udp u;
	void run()
	{
		char buf[4096];
		struct sockaddr_in peer;
		size_t len = sizeof(buf);
		ccf::udp::recv ur(echo_task::u, (struct sockaddr *)&peer, buf, len);
		await(ur);
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

int get_seq_from_buf(const void* buf, size_t size, const void** pos, size_t* len)
{
	if (size < sizeof(ccf::uint32))
		return -1;
	*pos = buf;
	*len = sizeof(ccf::uint32);
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
		*pos = this->seq;
		ccf::udp::send us(seq_task::u, seq_task::target, buf, sizeof(ccf::uint32));
		await(us);
		size_t len = sizeof(buf);
		ccf::udp::recv_by_seq ur(seq_task::u, NULL, buf, len, this->seq);
		await(ur);
		if (++seq_task::times == TEST_TIMES)
		{
			end = clock();
			cout << "Init: " << (cut - bgn) << endl;
			cout << "Proc: " << (end - cut) << endl;
			cout << "Total: " << (end - bgn) << endl;
			exit(0);
		}
		if ((seq_task::times+1)%100 == 99)
			ccf::sync::notify(0l);
	}
public:
	static void init()
	{
		seq_task::target = ccf::ip_to_addr("127.0.0.1", TEST_PORT);
		ASSERT(seq_task::u.bind(get_seq_from_buf) == 0);
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
				ccf::sync s(0l);
				await(s);
			}
			seq_task* seq = new seq_task(i);
			ASSERT(seq->status() == ccf::ready);
			ccf::start(seq);
		}
	}
};

int main()
{	
	bgn = clock();
	
	ccf::event_task::init(100);
	test_task::init(11000);
	main_task tMain;
	
	cut = clock();
	
	ccf::cocoflow(tMain);
	
	return 0;
}
