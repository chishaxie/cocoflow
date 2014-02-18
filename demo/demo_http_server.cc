#include <stdio.h>
#include <stdlib.h>

#include "cocoflow.h"

using namespace std;

static int port = 1337;

typedef ccf::task<7> my_task;

class http_task: public my_task
{
public:
	ccf::tcp::connected http_connection;
private:
	void run()
	{
		int ret;
		char req_buf[3072], res_buf[1024];
		size_t req_len = sizeof(req_buf), res_len = 0, tmp;
		ccf::tcp::recv_till recv_task(ret, http_connection, req_buf, req_len, "\r\n\r\n", 4);
		await(recv_task);
		if (ret == ccf::tcp::success)
		{
			do {
				tmp = res_len;
				res_len = sprintf(res_buf, "HTTP/1.0 200 OK\r\nServer: cocoflow-http-server\r\nContent-Type: text/plain\r\nContent-Length: %u\r\n\r\n", (unsigned)(req_len + tmp + 36));
			} while(res_len != tmp);
			ccf::tcp::send send_task(ret, http_connection, res_buf, res_len, req_buf, req_len, "--------------------------------\r\n\r\n", 36, res_buf, res_len);
			await(send_task);
		}
	}
};

class main_task: public my_task
{
	void run()
	{
		ccf::tcp::listening http_server;
		if (http_server.bind(ccf::ip_to_addr("0.0.0.0", port)) != 0)
		{
			perror("Bind socket");
			exit(1);
		}
		for (;;)
		{
			int ret;
			http_task *new_http_task = new http_task();
			ccf::tcp::accept accept_task(ret, http_server, new_http_task->http_connection);
			await(accept_task);
			if (ret != ccf::tcp::success)
				break;
			ccf::start(new_http_task);
		}
	}
};

int main(int argc, char *argv[])
{
	if (argc > 2)
		port = atoi(argv[1]);
	
	my_task::init(1025);
	ccf::event_task::init(4);
	
	main_task my_main;
	
	ccf::cocoflow(my_main);
	
	return 0;
}
