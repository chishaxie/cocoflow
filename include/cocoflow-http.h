#ifndef __COCOFLOW_HTTP_H__
#define __COCOFLOW_HTTP_H__

#include "cocoflow.h"

namespace ccf {

namespace http {

class get : public event_task
{
public:
	enum {
		err_unfinished = -1,
		err_url_parse = -2,
		err_dns_resolve = -3,
		err_connect = -4,
		err_request = -5,
		err_response = -6,
	};
	get(int &ret, const char **errmsg, const char *url, void *buf, size_t &len);
	virtual ~get();
private:
	get(const get&);
	get& operator=(const get&);
	virtual void run();
	virtual void cancel();
	int &ret;
	const char **errmsg;
	const char *url;
	void *buf;
	size_t &len;
};

} /* end of namespace http */

} /* end of namespace ccf */

#endif
