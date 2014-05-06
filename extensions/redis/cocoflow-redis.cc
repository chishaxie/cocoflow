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

/***** redis *****/

redis::redis()
	: context(NULL)
{
}

const char* redis::errstr()
{
	if (this->context)
		return reinterpret_cast<redisAsyncContext*>(this->context)->errstr;
	else
		return "Uninitialized";
}

redis::~redis()
{
	if (this->context)
		redisAsyncDisconnect(reinterpret_cast<redisAsyncContext*>(this->context));
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
