#include <iostream>

#include "cocoflow.h"
#include "cocoflow-redis.h"

#define TEST_PORT  6379
#define TEST_KEY   "test_key_for_cocoflow_redis"
#define TEST_VAL   "test_val_for_cocoflow_redis"

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
		
		r.auto_connect("127.0.0.1", TEST_PORT);
		
		{
			ccf::redis::command rset(&ret, &reply, r, "SET %s %s", TEST_KEY, TEST_VAL);
			await(rset);
			cout << "command(set): " << ret << endl;
			if (ret)
				cout << "errstr: " << r.errstr() << endl;
			else
				show_reply(reply);
			
			ccf::sleep s(1000);
			await(s);
		}
		
		for (;;)
		{
			ccf::redis::command rget(&ret, &reply, r, "GET %s", TEST_KEY);
			await(rget);
			cout << "command(get): " << ret << endl;
			if (ret)
				cout << "errstr: " << r.errstr() << endl;
			else
				show_reply(reply);
			
			ccf::sleep s(1000);
			await(s);
		}
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
