#ifndef __COCOFLOW_HTTP_H__
#define __COCOFLOW_HTTP_H__

#include "cocoflow.h"

namespace ccf {

namespace http {

//retcode
enum {
	OK                            = 200,
	unfinished                    = -1,
	err_url_parse                 = -2,
	err_dns_resolve               = -3,
	err_tcp_connect               = -4,
	err_send_request              = -5,
	err_recv_response             = -6,
	err_response_header_parse     = -7,
	err_response_body_parse       = -8,
	err_response_body_too_long    = -9,
	Continue                      = 100,
	SwitchingProtocols            = 101,
	Created                       = 201,
	Accepted                      = 202,
	NonAuthoritativeInformation   = 203,
	NoContent                     = 204,
	ResetContent                  = 205,
	PartialContent                = 206,
	MultipleChoices               = 300,
	MovedPermanently              = 301,
	Found                         = 302,
	SeeOther                      = 303,
	NotModified                   = 304,
	UseProxy                      = 305,
	TemporaryRedirect             = 307,
	BadRequest                    = 400,
	Unauthorized                  = 401,
	PaymentRequired               = 402,
	Forbidden                     = 403,
	NotFound                      = 404,
	MethodNotAllowed              = 405,
	NotAcceptable                 = 406,
	ProxyAuthenticationRequired   = 407,
	RequestTimeout                = 408,
	Conflict                      = 409,
	Gone                          = 410,
	LengthRequired                = 411,
	PreconditionFailed            = 412,
	RequestEntityTooLarge         = 413,
	RequestURITooLong             = 414,
	UnsupportedMediaType          = 415,
	RequestedRangeNotSatisfiable  = 416,
	ExpectationFailed             = 417,
	InternalServerError           = 500,
	NotImplemented                = 501,
	BadGateway                    = 502,
	ServiceUnavailable            = 503,
	GatewayTimeout                = 504,
	HTTPVersionNotSupported       = 505,
};

class get : public event_task
{
public:
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
