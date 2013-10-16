#include <string.h>
#include <time.h>

#include <iostream>

#include "cocoflow.h"

using namespace std;

#define TEST_PORT	31005
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

size_t get_len_from_header(const void* buf, size_t size)
{
	if (size < sizeof(ccf::uint32))
		return 0;
	const ccf::uint32 *len = (const ccf::uint32 *)buf;
	return *len;
}

int get_seq_from_buf(const void* buf, size_t size, const void** pos, size_t* len)
{
	if (size < sizeof(ccf::uint32) + sizeof(ccf::uint32))
		return -1;
	*pos = (const char *)buf + sizeof(ccf::uint32);
	*len = sizeof(ccf::uint32);
	return 0;
}

class echo_task: public test_task
{
	static int times;
	static ccf::tcp::connected tc;
	void run()
	{
		int ret;
		char buf[1024];
		size_t len = sizeof(buf);
		ccf::tcp::recv tr(ret, echo_task::tc, buf, len);
		await(tr);
		ASSERT(ret == ccf::tcp::success);
		if (++echo_task::times < TEST_TIMES)
		{
			echo_task* echo = new echo_task();
			ASSERT(echo->status() == ccf::ready);
			ccf::start(echo);
		}
		ccf::tcp::send ts(ret, echo_task::tc, buf, len);
		await(ts);
		ASSERT(ret == ccf::tcp::success);
	}
	friend class accept_task;
public:
	static void init()
	{
		ASSERT(echo_task::tc.bind(sizeof(ccf::uint32), 1024, get_len_from_header, get_seq_from_buf) == 0);
	}
};

int echo_task::times = 0;
ccf::tcp::connected echo_task::tc;

class accept_task: public test_task
{
	static ccf::tcp::listening tl;
	void run()
	{
		int ret;
		ASSERT(accept_task::tl.bind(ccf::ip_to_addr("0.0.0.0", TEST_PORT)) == 0);
		ccf::tcp::accept ta(ret, accept_task::tl, echo_task::tc);
		await(ta);
		ASSERT(ret == ccf::tcp::success);
		echo_task::init();
		ccf::start(new echo_task());
	}
};

ccf::tcp::listening accept_task::tl(1);

class seq_task: public test_task
{
	static int times;
	static ccf::tcp::connected tc;
	ccf::uint32 seq;
	void run()
	{
		int ret;
		char buf[1024];
		ccf::uint32 *plen = (ccf::uint32 *)buf;
		ccf::uint32 *pseq = ((ccf::uint32 *)buf) + 1;
		*plen = rand()%128 + 8;
		*pseq = this->seq;
		ccf::tcp::send ts(ret, seq_task::tc, buf, *plen);
		await(ts);
		ASSERT(ret == ccf::tcp::success);
		size_t len = sizeof(buf);
		ccf::tcp::recv_by_seq tr(ret, seq_task::tc, buf, len, this->seq);
		await(tr);
		ASSERT(ret == ccf::tcp::success);
		if (++seq_task::times == TEST_TIMES)
		{
			end = clock();
			cout << "Init: " << (cut - bgn) << endl;
			cout << "Proc: " << (end - cut) << endl;
			cout << "Total: " << (end - bgn) << endl;
			exit(0);
		}
	}
public:
	static void init()
	{
		int ret;
		ccf::tcp::connect c(ret, seq_task::tc, ccf::ip_to_addr("127.0.0.1", TEST_PORT));
		await(c);
		ASSERT(ret == ccf::tcp::success);
		ASSERT(seq_task::tc.bind(sizeof(ccf::uint32), 1024, get_len_from_header, get_seq_from_buf) == 0);
	}
	seq_task(ccf::uint32 seq) : seq(seq) {}
};

int seq_task::times = 0;
ccf::tcp::connected seq_task::tc;

class main_task: public test_task
{
	void run()
	{
		ccf::start(new accept_task());
		seq_task::init();
		for (int i=0; i<TEST_TIMES; i++)
		{
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
