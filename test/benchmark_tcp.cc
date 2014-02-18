#include <string.h>
#include <time.h>

#include <iostream>

#include "cocoflow.h"

#include "simple_rand.h"

using namespace std;

#define TEST_PORT	31005
#ifdef _WIN32
#define TEST_TIMES	300 //lite
#else
#define TEST_TIMES	6000
#endif

#ifdef _MSC_VER
#define ENSURE_TIMING 1 //Avoid unexpected tcp timing in Win
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

typedef ccf::task<15> test_task;

size_t get_len_from_header(const void* buf, size_t size)
{
	if (size < sizeof(ccf::uint32))
		return 0;
	const ccf::uint32 *len = (const ccf::uint32 *)buf;
	return *len;
}

int get_seq_from_buf(const void* buf, size_t size, ccf::uint32* seq)
{
	if (size < sizeof(ccf::uint32) + sizeof(ccf::uint32))
		return -1;
	*seq = ntohl(*(((ccf::uint32*)buf) + 1));
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
		ASSERT(echo_task::tc.bind(sizeof(ccf::uint32), 1024, get_len_from_header, get_seq_from_buf, my_pkg_seq_unrecv, my_pkg_seq_failed) == 0);
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
		int ret0, ret1;
		char buf_in[1024], buf_out[1024];
		ccf::uint32 *plen = (ccf::uint32 *)buf_out;
		ccf::uint32 *pseq = ((ccf::uint32 *)buf_out) + 1;
		*plen = simple_rand()%128 + 8;
		*pseq = htonl(this->seq);
		size_t len = sizeof(buf_in);
#ifndef ENSURE_TIMING
		ccf::tcp::send ts(ret0, seq_task::tc, buf_out, *plen);
		await(ts);
		ASSERT(ret0 == ccf::tcp::success);
		ccf::tcp::recv_by_seq_u32 tr(ret1, seq_task::tc, buf_in, len, this->seq);
		await(tr);
		ASSERT(ret1 == ccf::tcp::success);
#else
		ccf::tcp::send ts(ret0, seq_task::tc, buf_out, *plen);
		ccf::tcp::recv_by_seq_u32 tr(ret1, seq_task::tc, buf_in, len, this->seq);
		ccf::all_of all(ts, tr);
		await(all);
		ASSERT(ret0 == ccf::tcp::success);
		ASSERT(ret1 == ccf::tcp::success);
#endif
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
		int ret;
		ccf::tcp::connect c(ret, seq_task::tc, ccf::ip_to_addr("127.0.0.1", TEST_PORT));
		await(c);
		ASSERT(ret == ccf::tcp::success);
		ASSERT(seq_task::tc.bind(sizeof(ccf::uint32), 1024, get_len_from_header, get_seq_from_buf, my_pkg_seq_unrecv, my_pkg_seq_failed) == 0);
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
	time_bgn = clock();
	
#ifndef ENSURE_TIMING
	ccf::event_task::init(1);
#else
	ccf::event_task::init(TEST_TIMES * 2);
#endif
	test_task::init(TEST_TIMES * 2 + 1);
	main_task tMain;
	
	time_cut = clock();
	
	ccf::cocoflow(tMain);
	
	return 0;
}
