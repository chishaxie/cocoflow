#include <iostream>

#include "cocoflow.h"
#include "cocoflow-redis.h"

#define TEST_PORT  6379
#define TEST_KEY   "test_key_for_cocoflow_redis"
#define TEST_VAL   "test_val_for_cocoflow_redis"

using namespace std;

class main_task: public ccf::user_task
{
	void run()
	{
		int ret;
		ccf::redis r;
		ccf::redis::connect rc(&ret, r, "127.0.0.1", TEST_PORT);
		await(rc);
		cout << "connect: " << ret << endl;
		if (ret)
			cout << "errstr: " << r.errstr() << endl;
		ccf::redis::command* rset = new ccf::redis::command(NULL, NULL, r, "SET %s %s", TEST_KEY, TEST_VAL);
		start(rset);
		const redisReply *reply = NULL;
		ccf::redis::command rget(&ret, &reply, r, "GET %s", TEST_KEY);
		await(rget);
		cout << "command(get): " << ret << endl;
		if (ret)
			cout << "errstr: " << r.errstr() << endl;
		else
			cout << "str: " << reply->str << endl;
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
