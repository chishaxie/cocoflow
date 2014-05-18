#include <iostream>

#include "cocoflow.h"

using namespace std;

void show(int ret, struct addrinfo *result, const char *errmsg)
{
	if (ret)
	{
		if (errmsg)
			cout << "getaddrinfo: " << ret << " " << errmsg << endl;
		else
			cout << "getaddrinfo: " << ret << endl;
	}
	else
	{
		cout << "getaddrinfo:" << endl;
		struct addrinfo *cur = result;
		while (cur)
		{
			cout << "  " << ccf::ip_to_str(cur->ai_addr) << endl;
			cur = cur->ai_next;
		}
		ccf::getaddrinfo::freeaddrinfo(result);
	}
}

class main_task: public ccf::user_task
{
	void run()
	{
		int ret;
		struct addrinfo *result;
		const char *errmsg;
		
		ccf::getaddrinfo server(ret, &result, &errmsg, NULL, "ssh");
		await(server);
		show(ret, result, errmsg);
		
		ccf::getaddrinfo dns(ret, &result, &errmsg, "localhost", NULL);
		await(dns);
		show(ret, result, errmsg);
	
		ccf::getaddrinfo dns2(ret, &result, &errmsg, "www.qq.com", NULL);
		await(dns2);
		show(ret, result, errmsg);
	
		ccf::getaddrinfo dns3(ret, &result, &errmsg, "127.0.0.1", NULL);
		await(dns3);
		show(ret, result, errmsg);
		
		ccf::getaddrinfo none(ret, &result, &errmsg, NULL, NULL);
		await(none);
		show(ret, result, errmsg);
		
		ccf::getaddrinfo dns4(ret, &result, &errmsg, "www.qq.com", NULL);
		ccf::sleep s(0);
		ccf::any_of any(dns4, s);
		await(any);
		show(ret, result, errmsg);
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
