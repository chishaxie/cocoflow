#include "cocoflow-comm.h"

namespace ccf {

/***** sleep *****/

sleep::sleep(uint64 timeout)
	: timeout(timeout), timer(NULL)
{
}

static void sleep_cb(uv_timer_t* req, int status)
{
	CHECK(status == 0);
	__task_stand(reinterpret_cast<event_task*>(req->data));
}

void sleep::run()
{
	this->timer = malloc(sizeof(uv_timer_t));
	CHECK(this->timer != NULL);
	CHECK(uv_timer_init(loop(), reinterpret_cast<uv_timer_t*>(this->timer)) == 0);
	reinterpret_cast<uv_timer_t*>(this->timer)->data = this;
	CHECK(uv_timer_start(reinterpret_cast<uv_timer_t*>(this->timer), sleep_cb, this->timeout, 0) == 0);
	if (!__task_yield(this))
		return;
	uv_close(reinterpret_cast<uv_handle_t*>(this->timer), free_self_close_cb);
}

void sleep::cancel()
{
	uv_close(reinterpret_cast<uv_handle_t*>(this->timer), free_self_close_cb);
}

sleep::~sleep()
{
}

}
