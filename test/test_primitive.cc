#include <iostream>

#include "cocoflow.h"

using namespace std;

#define TEST_PORT	31007

typedef ccf::uint32 u32;

static u32 g_order = 0;

#define check(exp) \
do { \
	if (!(exp)) \
	{ \
		fprintf(stderr, "[Line: %u] expect " #exp "\n", __LINE__); \
		abort(); \
	} \
} while(0)

#define check_order(order) \
do { \
	if (++g_order != order) \
	{ \
		fprintf(stderr, "[Line: %u] expect %u, actual %u\n", __LINE__, order, g_order); \
		abort(); \
	} \
} while(0)

#define check_reset() \
do { \
	g_order = 0; \
} while(0)

const static struct sockaddr_in test_addr = ccf::ip_to_addr("127.0.0.1", TEST_PORT);

class await_test_bottom: public ccf::user_task
{
	void run()
	{
		ccf::sleep s(0);
		check_order(4);
		await(s);
		check_order(5);
	}
};

class await_test_middle: public ccf::user_task
{
	void run()
	{
		await_test_bottom atb;
		check_order(3);
		await(atb);
		check_order(6);
	}
};

class await_test_top: public ccf::user_task
{
	void run()
	{
		await_test_middle atm;
		check_order(2);
		await(atm);
		check_order(7);
	}
};

class test_no_block: public ccf::user_task
{
	void run() { check_order(doing); }
	void cancel() { check_order(0); }
	u32 destroy;
	u32 doing;
	u32 undo;
public:
	test_no_block(u32 create, u32 destroy, u32 doing)
		: destroy(destroy), doing(doing), undo(undo) { check_order(create); }
	~test_no_block() { check_order(destroy); }
};

class test_block: public ccf::user_task
{
	void run()
	{
		ccf::sleep s(0);
		check_order(bgn);
		await(s);
		check_order(end);
	}
	void cancel() { check_order(undo); }
	u32 destroy;
	u32 bgn;
	u32 end;
	u32 undo;
public:
	test_block(u32 create, u32 destroy, u32 bgn, u32 end, u32 undo)
		: destroy(destroy), bgn(bgn), end(end), undo(undo) { check_order(create); }
	~test_block() { check_order(destroy); }
};

class cancel_test_bottom: public ccf::user_task
{
	void run()
	{
		ccf::sleep s(1);
		check_order(3);
		await(s);
		check_order(0);
	}
	void cancel() { check_order(4); }
};

class cancel_test_top: public ccf::user_task
{
	void run()
	{
		cancel_test_bottom ctb;
		check_order(2);
		await(ctb);
		check_order(0);
	}
	void cancel() { check_order(5); }
};

class test_unsupport: public ccf::user_task
{
	void run()
	{
		ccf::udp u;
		ccf::udp::send us(u, test_addr, "test", 4);
		check_order(bgn);
		await(us);
		check_order(end);
	}
	void cancel() { check_order(undo); }
	u32 destroy;
	u32 bgn;
	u32 end;
	u32 undo;
public:
	test_unsupport(u32 create, u32 destroy, u32 bgn, u32 end, u32 undo)
		: destroy(destroy), bgn(bgn), end(end), undo(undo) { check_order(create); }
	~test_unsupport() { check_order(destroy); }
};

class check_unsupport: public ccf::user_task
{
	static ccf::udp u;
	void run()
	{
		char buf[4096];
		size_t len = sizeof(buf);
		ccf::udp::recv ur(check_unsupport::u, buf, len);
		await(ur);
		ccf::sleep s(20);
		await(s); //Ensure awiat sync
		check_order(end);
		ccf::sync::notify(0l);
	}
	u32 end;
public:
	static void init()
	{
		check(check_unsupport::u.bind(test_addr) == 0);
	}
	check_unsupport(u32 end)
		: end(end) {}
	~check_unsupport() {}
};

ccf::udp check_unsupport::u;

class test_sleep: public ccf::user_task
{
	void run()
	{
		ccf::sleep s(timeout);
		check_order(bgn);
		await(s);
		check_order(end);
	}
	void cancel() { check_order(undo); }
	u32 timeout;
	u32 bgn;
	u32 end;
	u32 undo;
public:
	test_sleep(u32 timeout, u32 bgn, u32 end, u32 undo)
		: timeout(timeout), bgn(bgn), end(end), undo(undo) {}
};

class main_task: public ccf::user_task
{
	void run()
	{
		//await layer
		{
			check_order(1);
			{
				await_test_top att;
				await(att);
			}
			check_order(8);
		}
		check_reset();
		//await no block
		{
			check_order(1);
			{
				test_no_block tnb(2, 4, 3);
				await(tnb);
			}
			check_order(5);
		}
		//start no block
		check_reset();
		{
			check_order(1);
			{
				ccf::start(new test_no_block(2, 4, 3));
			}
			check_order(5);
		}
		//all_of no block
		check_reset();
		{
			check_order(1);
			{
				test_no_block tnb0(2, 7, 4);
				test_no_block tnb1(3, 6, 5);
				ccf::all_of all(tnb0, tnb1);
				await(all);
				check(tnb0.status() == ccf::completed);
				check(tnb1.status() == ccf::completed);
			}
			check_order(8);
		}
		//any_of no block
		check_reset();
		{
			check_order(1);
			{
				test_no_block tnb0(2, 6, 4);
				test_no_block tnb1(3, 5, 0);
				ccf::any_of any(tnb0, tnb1);
				await(any);
				check(tnb0.status() == ccf::completed);
				check(tnb1.status() == ccf::ready);
				check(any.who_completed() == 0);
			}
			check_order(7);
		}
		//all_of block
		check_reset();
		{
			check_order(1);
			{
				test_block tb0(2, 9, 4, 6, 0);
				test_block tb1(3, 8, 5, 7, 0);
				ccf::all_of all(tb0, tb1);
				await(all);
				check(tb0.status() == ccf::completed);
				check(tb1.status() == ccf::completed);
			}
			check_order(10);
		}
		//any_of block
		check_reset();
		{
			check_order(1);
			{
				test_block tb0(2, 9, 4, 6, 0);
				test_block tb1(3, 8, 5, 0 ,7);
				ccf::any_of any(tb0, tb1);
				await(any);
				check(tb0.status() == ccf::completed);
				check(tb1.status() == ccf::canceled);
				check(any.who_completed() == 0);
			}
			check_order(10);
		}
		//all_of sleep
		check_reset();
		{
			ccf::sleep s0(1);
			ccf::sleep s1(0);
			ccf::all_of all(s0, s1);
			await(all);
			check(s0.status() == ccf::completed);
			check(s1.status() == ccf::completed);
		}
		//any_of sleep
		check_reset();
		{
			ccf::sleep s0(1);
			ccf::sleep s1(0);
			ccf::any_of any(s0, s1);
			await(any);
			check(s0.status() == ccf::canceled);
			check(s1.status() == ccf::completed);
			check(any.who_completed() == 1);
		}
		check_reset();
		//any_of layer cancel
		{
			check_order(1);
			{
				cancel_test_top ctt;
				ccf::sleep s(0);
				ccf::any_of any(ctt, s);
				await(any);
				check(ctt.status() == ccf::canceled);
				check(s.status() == ccf::completed);
				check(any.who_completed() == 1);
			}
			check_order(6);
		}
		check_reset();
		//any_of cancel all_of
		{
			check_order(1);
			{
				ccf::sleep s0(0), s1(1), s2(0);
				ccf::all_of all(s0, s1);
				ccf::any_of any(all, s2);
				await(any);
				check(s0.status() == ccf::completed);
				check(s1.status() == ccf::canceled);
				check(s2.status() == ccf::completed);
				check(all.status() == ccf::canceled);
				check(any.who_completed() == 1);
			}
			check_order(2);
		}
		check_reset();
		//any_of cancel any_of
		{
			check_order(1);
			{
				ccf::sleep s0(0), s1(1);
				test_no_block tnb(2, 4, 3);
				ccf::any_of any0(s0, s1);
				ccf::any_of any(any0, tnb);
				await(any);
				check(s0.status() == ccf::canceled);
				check(s1.status() == ccf::canceled);
				check(tnb.status() == ccf::completed);
				check(any0.status() == ccf::canceled);
				check(any.who_completed() == 1);
			}
			check_order(5);
		}
		check_reset();
		check_unsupport::init();
		//any_of cancel unsupport-cancel
		{
			check_order(1);
			{
				ccf::udp u;
				ccf::udp::send us(u, test_addr, "test", 4);
				test_no_block tnb(2, 5, 3);
				ccf::any_of any(us, tnb);
				ccf::start(new check_unsupport(4));
				await(any);
				check(us.status() == ccf::completed);
				check(tnb.status() == ccf::completed);
				check(any.who_completed() == 1);
				ccf::sync s(0l);
				await(s);
			}
			check_order(6);
		}
		check_reset();
		//any_of cancel test_unsupport
		{
			check_order(1);
			{
				test_unsupport tu(2, 9, 4, 0, 6);
				test_no_block tnb(3, 8, 5);
				ccf::any_of any(tu, tnb);
				ccf::start(new check_unsupport(7));
				await(any);
				check(tu.status() == ccf::canceled);
				check(tnb.status() == ccf::completed);
				check(any.who_completed() == 1);
				ccf::sync s(0l);
				await(s);
			}
			check_order(10);
		}
		check_reset();
		//any_of cancel sleep(uninterruptable) in any_of
		{
			check_order(1);
			{
				ccf::sleep s0(20);
				ccf::sleep s1(40);
				ccf::sleep s2(60);
				ccf::sleep s3(80);
				s2.uninterruptable();
				ccf::any_of any0(s0, s2, s3);
				ccf::any_of any1(any0, s1);
				await(any1);
				check(s0.status() == ccf::completed);
				check(s1.status() == ccf::completed);
				check(s2.status() == ccf::completed);
				check(s3.status() == ccf::canceled);
				check(any0.status() == ccf::completed);
				check(any0.who_completed() == 0);
				check(any1.who_completed() == 1);
			}
			check_order(2);
		}
		check_reset();
		//any_of cancel test_sleep(uninterruptable) in any_of
		{
			check_order(1);
			{
				test_sleep ts0(20, 2, 6, 0);
				test_sleep ts1(40, 5, 8, 0);
				test_sleep ts2(60, 3, 9, 0);
				test_sleep ts3(80, 4, 0, 7);
				ts2.uninterruptable();
				ccf::any_of any0(ts0, ts2, ts3);
				ccf::any_of any1(any0, ts1);
				await(any1);
				check(ts0.status() == ccf::completed);
				check(ts1.status() == ccf::completed);
				check(ts2.status() == ccf::completed);
				check(ts3.status() == ccf::canceled);
				check(any0.status() == ccf::completed);
				check(any0.who_completed() == 0);
				check(any1.who_completed() == 1);
			}
			check_order(10);
		}
		check_reset();
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
