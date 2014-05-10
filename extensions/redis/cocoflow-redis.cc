#include "cocoflow-comm.h"

extern "C" {
#define __disable_simplify_hiredis_h__
#include "adapters/libuv.h"
}

#include "cocoflow-redis.h"

#ifndef REDIS_UNFINISHED
#define REDIS_UNFINISHED -2
#endif

namespace ccf {

#define RECONNECT_INTERVAL_MIN  10
#define RECONNECT_INTERVAL_MAX  10000

/***** redis *****/

redis::redis()
	: context(NULL), timer(NULL), cur_reconnect_interval(0), timeout(0),
	  succeed(NULL), failed(NULL), data(NULL)
{
}

const char* redis::errstr()
{
	if (this->context)
		return reinterpret_cast<redisAsyncContext*>(this->context)->errstr;
	else
		return "Uninitialized";
}

int redis::auto_connect(const char* ip, int port, int timeout)
{
	CHECK(this->context == NULL);
	CHECK(ip != NULL);
	CHECK(timeout > 0);
	this->ip = ip;
	this->port = port;
	this->timeout = timeout;
	return this->connect_now(false);
}

void redis::set_auto_connect_callback(auto_connect_succeed* succeed, auto_connect_failed* failed, void* data)
{
	this->succeed = succeed;
	this->failed = failed;
	this->data = data;
}

redis::~redis()
{
	if (this->context)
	{
		reinterpret_cast<redisAsyncContext*>(this->context)->data = NULL;
		redisAsyncFree(reinterpret_cast<redisAsyncContext*>(this->context));
		if (this->timer)
			uv_close(reinterpret_cast<uv_handle_t*>(this->timer), free_self_close_cb);
	}
}

int redis::connect_now(bool always_return_zero)
{
	this->context = redisAsyncConnect(this->ip.c_str(), this->port);
	if (always_return_zero)
		CHECK(reinterpret_cast<redisAsyncContext*>(this->context)->err == REDIS_OK);
	else if (reinterpret_cast<redisAsyncContext*>(this->context)->err != REDIS_OK)
		return reinterpret_cast<redisAsyncContext*>(this->context)->err;
	CHECK(redisLibuvAttach(reinterpret_cast<redisAsyncContext*>(this->context), loop()) == REDIS_OK);
	reinterpret_cast<redisAsyncContext*>(this->context)->data = this;
	CHECK(redisAsyncSetConnectCallback(reinterpret_cast<redisAsyncContext*>(this->context), redis::auto_connect_cb) == REDIS_OK);
	CHECK(redisAsyncSetDisconnectCallback(reinterpret_cast<redisAsyncContext*>(this->context), redis::auto_connect_closed_cb) == REDIS_OK);
	
	this->timer = malloc(sizeof(uv_timer_t));
	CHECK(this->timer != NULL);
	CHECK(uv_timer_init(loop(), reinterpret_cast<uv_timer_t*>(this->timer)) == 0);
	reinterpret_cast<uv_timer_t*>(this->timer)->data = this;
	CHECK(uv_timer_start(reinterpret_cast<uv_timer_t*>(this->timer), redis::auto_connect_timeout_cb, this->timeout, 0) == 0);
	return 0;
}

int redis::connect_coming(bool old_opened)
{
	this->old_opened = old_opened;
	if (this->cur_reconnect_interval == 0)
		this->cur_reconnect_interval = RECONNECT_INTERVAL_MIN;
	else
		this->cur_reconnect_interval *= 2;
	if (this->cur_reconnect_interval > RECONNECT_INTERVAL_MAX)
		this->cur_reconnect_interval = RECONNECT_INTERVAL_MAX;
	this->timer = malloc(sizeof(uv_timer_t));
	CHECK(this->timer != NULL);
	CHECK(uv_timer_init(loop(), reinterpret_cast<uv_timer_t*>(this->timer)) == 0);
	reinterpret_cast<uv_timer_t*>(this->timer)->data = this;
	CHECK(uv_timer_start(reinterpret_cast<uv_timer_t*>(this->timer), redis::auto_reconnect_next_cb, this->cur_reconnect_interval, 0) == 0);
	return 0;
}

void redis::auto_connect_cb(const redisAsyncContext *c, int status)
{
	redis* _this = reinterpret_cast<redis*>(c->data);
	uv_close(reinterpret_cast<uv_handle_t*>(_this->timer), free_self_close_cb);
	if (status == REDIS_OK)
	{
		_this->cur_reconnect_interval = 0;
		if (_this->succeed)
			_this->succeed(*_this, _this->data);
	}
	else
	{
		if (_this->failed)
			_this->failed(*_this, _this->data, redis::failed_exception, "an error occurred on connecting");
		_this->connect_coming(true);
	}
}

void redis::auto_connect_closed_cb(const redisAsyncContext *c, int status)
{
	if (reinterpret_cast<redis*>(c->data) != NULL)
	{
		redis* _this = reinterpret_cast<redis*>(c->data);
		if (_this->failed)
			_this->failed(*_this, _this->data, redis::failed_disconnect, "connection is closed by peer");
		_this->connect_coming(false);
	}
}

void redis::auto_connect_timeout_cb(uv_timer_t* req, int status)
{
	redis* _this = reinterpret_cast<redis*>(req->data);
	uv_close(reinterpret_cast<uv_handle_t*>(_this->timer), free_self_close_cb);
	if (_this->failed)
		_this->failed(*_this, _this->data, redis::failed_timeout, "connecting timeout");
	_this->connect_coming(true);
}

void redis::auto_reconnect_next_cb(uv_timer_t* req, int status)
{
	redis* _this = reinterpret_cast<redis*>(req->data);
	uv_close(reinterpret_cast<uv_handle_t*>(_this->timer), free_self_close_cb);
	if (_this->old_opened) //Delay free (handle always valid)
	{
		reinterpret_cast<redisAsyncContext*>(_this->context)->data = NULL;
		redisAsyncFree(reinterpret_cast<redisAsyncContext*>(_this->context));
	}
	_this->connect_now(true);
}

/***** redis.connect *****/

redis::connect::connect(int* ret, redis& handle, const char* ip, int port)
	: ret(ret), handle(handle), ip(strdup(ip)), port(port)
{
	CHECK(this->ip != NULL);
	if (this->ret)
		*this->ret = REDIS_UNFINISHED;
	this->uninterruptable();
}

void redis::connect_cb(const redisAsyncContext *c, int status)
{
	if (reinterpret_cast<redis::connect*>(c->data)->ret)
		*reinterpret_cast<redis::connect*>(c->data)->ret = status;
	__task_stand(reinterpret_cast<event_task*>(c->data));
}

void redis::connect::run()
{
	CHECK(this->handle.context == NULL);
	this->handle.context = redisAsyncConnect(this->ip, this->port);
	if (reinterpret_cast<redisAsyncContext*>(this->handle.context)->err != REDIS_OK)
	{
		if (this->ret)
			*this->ret = reinterpret_cast<redisAsyncContext*>(this->handle.context)->err;
		return;
	}
	CHECK(redisLibuvAttach(reinterpret_cast<redisAsyncContext*>(this->handle.context), loop()) == REDIS_OK);
	reinterpret_cast<redisAsyncContext*>(this->handle.context)->data = this;
	CHECK(redisAsyncSetConnectCallback(reinterpret_cast<redisAsyncContext*>(this->handle.context), redis::connect_cb) == REDIS_OK);
	(void)__task_yield(this);
}

void redis::connect::cancel()
{
	CHECK(0);
}

redis::connect::~connect()
{
	free(this->ip);
}

/***** redis.command *****/

redis::command::command(int* ret, const redisReply** reply, redis& handle, const char* format, ...)
	: ret(ret), reply(reply), handle(handle), req((redis::command**)malloc(sizeof(redis::command*)))
{
	CHECK(this->req != NULL);
	*this->req = this;
	if (this->ret)
		*this->ret = REDIS_UNFINISHED;
	va_list ap;
	va_start(ap, format);
	this->redis_err = redisvAsyncCommand(reinterpret_cast<redisAsyncContext*>(this->handle.context), redis::command_cb, this->req, format, ap);
	va_end(ap);
	if (this->redis_err != REDIS_OK)
	{
		free(this->req);
		this->req = NULL;
	}
}

redis::command::command(int* ret, const redisReply** reply, redis& handle, int argc, const char** argv, const size_t* argvlen)
	: ret(ret), reply(reply), handle(handle), req((redis::command**)malloc(sizeof(redis::command*)))
{
	CHECK(this->req != NULL);
	*this->req = this;
	if (this->ret)
		*this->ret = REDIS_UNFINISHED;
	this->redis_err = redisAsyncCommandArgv(reinterpret_cast<redisAsyncContext*>(this->handle.context), redis::command_cb, this->req, argc, argv, argvlen);
	if (this->redis_err != REDIS_OK)
	{
		free(this->req);
		this->req = NULL;
	}
}

void redis::command_cb(struct redisAsyncContext* c, void* r, void* privdata)
{
	redis::command** req = reinterpret_cast<redis::command**>(privdata);
	if (*req)
	{
		if ((*req)->status() == running)
		{
			if (r)
			{
				if ((*req)->reply)
					*(*req)->reply = reinterpret_cast<redisReply*>(r);
				if ((*req)->ret)
					*(*req)->ret = REDIS_OK;
			}
			else if ((*req)->ret)
				*(*req)->ret = REDIS_ERR;
			__task_stand(reinterpret_cast<event_task*>(*req));
		}
		else
			*req = NULL;
	}
	free(privdata);
}

void redis::command::run()
{
	if (this->redis_err != REDIS_OK)
	{
		if (this->ret)
			*this->ret = this->redis_err;
		return;
	}
	CHECK(*this->req != NULL);
	if (!__task_yield(this))
		return;
	*this->req = NULL;
}

void redis::command::cancel()
{
	*this->req = NULL;
}

redis::command::~command()
{
}

}
