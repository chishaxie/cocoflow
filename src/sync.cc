#include "cocoflow-comm.h"

namespace ccf {

/***** sync *****/

sync::sync()
	: async(NULL), named(false)
{
}

sync::sync(long id)
	: id(id), async(NULL), named(true)
{
}

static void sync_cb(uv_async_t* async, int status)
{
	CHECK(status == 0);
	__task_stand(reinterpret_cast<event_task*>(async->data));
}

void sync::run()
{
	if (this->named)
	{
		std::pair<std::map<long, sync*>::iterator, bool> ret = sync::ids.insert(std::pair<long, sync*>(this->id, this));
		CHECK(ret.second == true);
		this->pos = ret.first;
	}
	this->async = malloc(sizeof(uv_async_t));
	CHECK(this->async != NULL);
	CHECK(uv_async_init(loop(), reinterpret_cast<uv_async_t*>(this->async), sync_cb) == 0);
	reinterpret_cast<uv_async_t*>(this->async)->data = this;
	if (!__task_yield(reinterpret_cast<event_task*>(this)))
		return;
	if (this->named)
		sync::ids.erase(this->pos);
	uv_close(reinterpret_cast<uv_handle_t*>(this->async), free_self_close_cb);
	this->async = NULL;
}

void sync::cancel()
{
	if (this->named)
		sync::ids.erase(this->pos);
	uv_close(reinterpret_cast<uv_handle_t*>(this->async), free_self_close_cb);
	this->async = NULL;
}

sync::~sync()
{
}

int sync::notify(sync* obj)
{
	if (obj->async)
	{
		CHECK(uv_async_send(reinterpret_cast<uv_async_t*>(obj->async)) == 0);
		return 0;
	}
	else
		return -1;
}

int sync::notify(long id)
{
	std::map<long, sync*>::iterator it = sync::ids.find(id);
	if (it != sync::ids.end())
	{
		CHECK(uv_async_send(reinterpret_cast<uv_async_t*>(it->second->async)) == 0);
		return 0;
	}
	else
		return -1;
}

std::map<long, sync*> sync::ids;

}
