#include <stdlib.h>

#include <iostream>

#include "cocoflow.h"
#include "cocoflow-http.h"

using namespace std;

static char *url = NULL;
static char *buf = NULL;
static size_t size = 1048576;

class main_task: public ccf::user_task
{
	void run()
	{
		int ret;
		const char *errmsg;
		
		ccf::http::get GET(ret, &errmsg, url, buf, size);
		await(GET);
		
		cerr << "Status Code: " << ret << endl;
		if (errmsg)
			cerr << "Reason Phrase: " << errmsg << endl;
		cerr << "Body Length: " << size << endl;
		if (size)
		{
			buf[size] = '\0';
			cout << buf << endl;
		}
	}
};

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		cerr << "Usage: " << argv[0] << " <url> [bufsize=1048576]" << endl;
		return 1;
	}
	
	url = argv[1];
	if (argc > 2)
		size = atoi(argv[2]);
	
	if (size)
	{
		buf = reinterpret_cast<char *>(malloc(size + 1));
		if (!buf)
			return 2;
	}
	
	ccf::event_task::init(100);
	ccf::user_task::init(100);
	
	//ccf::set_debug(stderr);
	
	main_task tMain;
	ccf::cocoflow(tMain);
	return 0;
}
