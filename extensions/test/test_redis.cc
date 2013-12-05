#include <iostream>

#include "cocoflow.h"
#include "cocoflow-redis.h"

#define TEST_PORT  6379
#define TEST_KEY   "test_key_for_cocoflow_redis"
#define TEST_VAL   "test_val_for_cocoflow_redis"
#define TEST_KEY2  "test key for cocoflow redis"
#define TEST_VAL2  "test val for cocoflow redis"

using namespace std;

static void show_reply(const redisReply *reply)
{
	switch (reply->type)
	{
	case REDIS_REPLY_STRING:
		cout << "str: " << reply->str << endl;
		break;
	case REDIS_REPLY_ARRAY:
		cout << "array: (size=" << reply->elements << ")" << endl;
		break;
	case REDIS_REPLY_INTEGER:
		cout << "int: " << reply->integer << endl;
		break;
	case REDIS_REPLY_NIL:
		cout << "nil" << endl;
		break;
	case REDIS_REPLY_STATUS:
		cout << "status: " << reply->str << endl;
		break;
	case REDIS_REPLY_ERROR:
		cout << "error: " << reply->str << endl;
		break;
	default:
		cout << "bug" << endl;
		break;
	}
};

class main_task: public ccf::user_task
{
	void run()
	{
		int ret;
		const redisReply *reply;
		
		ccf::redis r;
		
		ccf::redis::connect rc(&ret, r, "127.0.0.1", TEST_PORT);
		await(rc);
		cout << "connect: " << ret << endl;
		if (ret)
			cout << "errstr: " << r.errstr() << endl;
		
		ccf::redis::command* rset = new ccf::redis::command(NULL, NULL, r, "SET %s %s", TEST_KEY, TEST_VAL);
		start(rset);
		
		ccf::redis::command rget(&ret, &reply, r, "GET %s", TEST_KEY);
		await(rget);
		cout << "command(get): " << ret << endl;
		if (ret)
			cout << "errstr: " << r.errstr() << endl;
		else
			show_reply(reply);
		
		const char *argv[] = {
			"SET",
			TEST_KEY2,
			TEST_VAL2
		};
		ccf::redis::command rset2(&ret, &reply, r, sizeof(argv)/sizeof(argv[0]), argv, NULL);
		await(rset2);
		cout << "command(set): " << ret << endl;
		if (ret)
			cout << "errstr: " << r.errstr() << endl;
		else
			show_reply(reply);
		
		const char *argv2[] = {
			"GET",
			TEST_KEY2
		};
		ccf::redis::command rget2(&ret, &reply, r, sizeof(argv2)/sizeof(argv2[0]), argv2, NULL);
		await(rget2);
		cout << "command(get): " << ret << endl;
		if (ret)
			cout << "errstr: " << r.errstr() << endl;
		else
			show_reply(reply);
		
		ccf::redis::command rget3(&ret, &reply, r, 1, argv2, NULL); //test error
		await(rget3);
		cout << "command(get): " << ret << endl;
		if (ret)
			cout << "errstr: " << r.errstr() << endl;
		else
			show_reply(reply);
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
