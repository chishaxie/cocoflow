#include "cocoflow-comm.h"

namespace ccf {

/***** getaddrinfo *****/

getaddrinfo::getaddrinfo(int& ret, struct addrinfo** result, const char** errmsg,
	const char* node, const char* service, const struct addrinfo* hints)
	: req(NULL), ret(ret), result(result), errmsg(errmsg), node(node), service(service), hints(hints)
{
	this->ret = -1;
	if (this->result)
		*this->result = NULL;
	if (this->errmsg)
		*this->errmsg = NULL;
}

void getaddrinfo::getaddrinfo_cb(uv_getaddrinfo_t* req, int status, struct addrinfo* result)
{
	if (status == 0)
	{
		if (req->data)
		{
			reinterpret_cast<getaddrinfo*>(req->data)->ret = 0;
			if (reinterpret_cast<getaddrinfo*>(req->data)->result)
				*reinterpret_cast<getaddrinfo*>(req->data)->result = result;
			else
				uv_freeaddrinfo(result);
			__task_stand(reinterpret_cast<event_task*>(req->data));
		}
		else
			uv_freeaddrinfo(result);
	}
	else
	{
		CHECK(result == NULL);
		if (req->data)
		{
			reinterpret_cast<getaddrinfo*>(req->data)->ret = status;
			if (reinterpret_cast<getaddrinfo*>(req->data)->errmsg)
				*reinterpret_cast<getaddrinfo*>(req->data)->errmsg = uv_strerror(uv_last_error(loop()));
			__task_stand(reinterpret_cast<event_task*>(req->data));
		}
	}
	free(req);
}

void getaddrinfo::run()
{
	this->req = malloc(sizeof(uv_getaddrinfo_t));
	CHECK(this->req != NULL);
	int status = uv_getaddrinfo(loop(), reinterpret_cast<uv_getaddrinfo_t*>(this->req), getaddrinfo::getaddrinfo_cb,
		this->node, this->service, this->hints);
	if (status == 0)
	{
		reinterpret_cast<uv_getaddrinfo_t*>(this->req)->data = this;
		(void)__task_yield(this);
	}
	else
	{
		this->ret = status;
		if (this->errmsg)
			*this->errmsg = uv_strerror(uv_last_error(loop()));
		free(this->req);
	}
}

void getaddrinfo::cancel()
{
	if (this->errmsg)
		*this->errmsg = "It was canceled";
	(void)uv_cancel(reinterpret_cast<uv_req_t*>(this->req)); //may fail in linux, must fail in win
	reinterpret_cast<uv_getaddrinfo_t*>(this->req)->data = NULL;
}

getaddrinfo::~getaddrinfo()
{
}

void getaddrinfo::freeaddrinfo(struct addrinfo* result)
{
	uv_freeaddrinfo(result);
}

}
