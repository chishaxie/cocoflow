#include <iostream>

#include "cocoflow.h"

using namespace std;

class test_task: public ccf::user_task
{
	int id;
	void run()
	{
		cout << this->id << " await sleep(3000)" << endl;
		ccf::sleep s1(1000), s2(2000);
		await(s1);
		await(s2);
		cout << this->id << " await End" << endl;
	}
public:
	test_task(int id):id(id){}
};

class main_task: public ccf::user_task
{
	void run()
	{
		cout << "await test_task(0:new)" << endl;
		test_task tt(0);
		await(tt);
		cout << "start test_task(0:end, 1:new)" << endl;
		ccf::start(new test_task(1));
		cout << "start test_task(1:end, 2:new)" << endl;
		ccf::start(new test_task(2));
		cout << "start test_task(2:end, 3:new)" << endl;
		ccf::start(new test_task(3));
		cout << "start test_task(3:end, 4:new)" << endl;
		ccf::start(new test_task(4));
		cout << "start test_task(4:end)" << endl;
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
