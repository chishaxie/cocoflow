#include "cocoflow-comm.h"

namespace ccf {

/***** udp *****/

udp::udp()
	: sock(malloc(sizeof(uv_udp_t))), receiving(0), cur_alloc(NULL), seqer(NULL),
	  unrecv(NULL), failed(NULL), c_unrecv(0), c_failed(0)
{
	CHECK(this->sock != NULL);
	CHECK(uv_udp_init(loop(), reinterpret_cast<uv_udp_t*>(this->sock)) == 0);
	reinterpret_cast<uv_udp_t*>(this->sock)->data = this;
}

int udp::bind(const struct sockaddr_in& addr)
{
	return uv_udp_bind(reinterpret_cast<uv_udp_t*>(this->sock), addr, 0);
}

int udp::bind(seq_getter* seqer)
{
	if (!seqer || this->seqer)
		return -1;
	this->seqer = seqer;
	return 0;
}

int udp::bind(pkg_seq_unrecv* unrecv)
{
	if (!unrecv || this->unrecv)
		return -1;
	this->unrecv = unrecv;
	return 0;
}

int udp::bind(pkg_seq_failed* failed)
{
	if (!failed || this->failed)
		return -1;
	this->failed = failed;
	return 0;
}

unsigned long long udp::count_unrecv() const
{
	return this->c_unrecv;
}

unsigned long long udp::count_failed() const
{
	return this->c_failed;
}

udp::~udp()
{
	if (this->receiving)
		CHECK(uv_udp_recv_stop(reinterpret_cast<uv_udp_t*>(this->sock)) == 0);
	uv_close(reinterpret_cast<uv_handle_t*>(this->sock), free_self_close_cb);
	reinterpret_cast<uv_udp_t*>(this->sock)->data = NULL;
}

const void* udp::internal_buffer(size_t& len)
{
	len = udp::routing_len;
	return udp::routing_buf;
}

char udp::routing_buf[65536];
size_t udp::routing_len = 0;

/***** udp.send *****/

static void udp_send_cb(uv_udp_send_t* req, int status)
{
	CHECK(status == 0);
	__task_stand(reinterpret_cast<event_task*>(req->data));
}

udp::send::send(udp& handle, const struct sockaddr_in& addr, const void* buf, size_t len)
	: handle(handle), addr(addr),
	  buf(uv_buf_init(reinterpret_cast<char*>(const_cast<void*>(buf)), static_cast<unsigned int>(len)))
{
	CHECK(buf != NULL && len > 0);
	this->uninterruptable();
}

void udp::send::run()
{
	uv_udp_send_t req;
	CHECK(uv_udp_send(&req, reinterpret_cast<uv_udp_t*>(this->handle.sock), &this->buf, 1, this->addr, udp_send_cb) == 0);
	req.data = this;
	(void)__task_yield(reinterpret_cast<event_task*>(this));
}

void udp::send::cancel()
{
	CHECK(0);
}

udp::send::~send()
{
}

/***** udp.recv *****/

uv_buf_t udp::udp_alloc_cb0(uv_handle_t* handle, size_t suggested_size)
{
	udp* obj = reinterpret_cast<udp*>(handle->data);
	CHECK(obj->cur_alloc == NULL);
	std::list<udp::recv*>::iterator it = obj->recv_queue.begin();
	CHECK(it != obj->recv_queue.end());
	obj->cur_alloc = *it;
	obj->recv_queue.erase(it);
	if (obj->cur_alloc->buf && obj->cur_alloc->len)
		return uv_buf_init(reinterpret_cast<char*>(obj->cur_alloc->buf), obj->cur_alloc->len);
	else
		return uv_buf_init(udp::routing_buf, sizeof(udp::routing_buf));
}

void udp::udp_recv_cb0(uv_udp_t* handle, ssize_t nread, uv_buf_t buf, struct sockaddr* addr, unsigned flags)
{
	udp* obj = reinterpret_cast<udp*>(handle->data);
	CHECK(obj->cur_alloc != NULL);
	CHECK(nread >= 0);
	if (nread == 0)
	{
		obj->cur_alloc->pos = obj->recv_queue.insert(obj->recv_queue.begin(), obj->cur_alloc);
		obj->cur_alloc = NULL;
	}
	else
	{
		if (obj->cur_alloc->addr)
			*obj->cur_alloc->addr = *addr;
		obj->cur_alloc->len = nread;
		event_task* target = obj->cur_alloc;
		obj->cur_alloc = NULL;
		__task_stand(target);
		if (obj == reinterpret_cast<udp*>(handle->data)) //Check valid
		{
			if (obj->recv_queue.empty())
			{
				CHECK(uv_udp_recv_stop(reinterpret_cast<uv_udp_t*>(obj->sock)) == 0);
				obj->receiving = 0;
			}
		}
	}
}

udp::recv::recv(udp& handle, struct sockaddr* addr, void* buf, size_t& len)
	: handle(handle), addr(addr), buf(buf), len(len)
{
}

void udp::recv::run()
{
	if (!this->handle.receiving)
	{
		if (!this->handle.seqer)
		{
			this->handle.receiving = 1;
			CHECK(uv_udp_recv_start(reinterpret_cast<uv_udp_t*>(this->handle.sock), udp::udp_alloc_cb0, udp::udp_recv_cb0) == 0);
		}
		else
		{
			this->handle.receiving = 2;
			CHECK(uv_udp_recv_start(reinterpret_cast<uv_udp_t*>(this->handle.sock), udp::udp_alloc_cb1, udp::udp_recv_cb1) == 0);
		}
	}
	this->pos = this->handle.recv_queue.insert(this->handle.recv_queue.end(), this);
	(void)__task_yield(reinterpret_cast<event_task*>(this));
}

void udp::recv::cancel()
{
	this->handle.recv_queue.erase(this->pos);
	if (this->handle.receiving == 1 && this->handle.recv_queue.empty())
	{
		CHECK(uv_udp_recv_stop(reinterpret_cast<uv_udp_t*>(this->handle.sock)) == 0);
		this->handle.receiving = 0;
	}
}

udp::recv::~recv()
{
}

/***** udp.recv_by_seq *****/

uv_buf_t udp::udp_alloc_cb1(uv_handle_t* handle, size_t suggested_size)
{
	return uv_buf_init(udp::routing_buf, sizeof(udp::routing_buf));
}

void udp::udp_recv_cb1(uv_udp_t* handle, ssize_t nread, uv_buf_t buf, struct sockaddr* addr, unsigned flags)
{
	CHECK(nread >= 0);
	if (nread == 0)
		return;
	udp* obj = reinterpret_cast<udp*>(handle->data);
	udp::routing_len = nread;
	sequence seq(NULL, 0);
	int ret = obj->seqer(udp::routing_buf, nread, &seq.seq, &seq.len);
	if (ret >= 0)
	{
		std::multimap<sequence, udp::recv_by_seq*>::iterator it = obj->seq_mapping.find(seq);
		if (it != obj->seq_mapping.end())
		{
			if (it->second->addr)
				*it->second->addr = *addr;
			if (it->second->buf)
			{
				if (it->second->len > udp::routing_len)
					it->second->len = udp::routing_len;
				if (it->second->len)
					memcpy(it->second->buf, udp::routing_buf, it->second->len);
			}
			else
				it->second->len = udp::routing_len;
			event_task* target = reinterpret_cast<event_task*>(it->second);
			obj->seq_mapping.erase(it);
			__task_stand(target);
		}
		else
		{
			if (obj->recv_queue.empty())
			{
				obj->c_unrecv ++;
				if (obj->unrecv)
					obj->unrecv(udp::routing_buf, udp::routing_len, seq.seq, seq.len);
			}
			else
				goto ____udp_recv_cb_rqne;
		}
	}
	else
	{
		if (obj->recv_queue.empty())
		{
			obj->c_failed ++;
			if (obj->failed)
				obj->failed(udp::routing_buf, udp::routing_len, ret);
		}
		else
		{
____udp_recv_cb_rqne:
			std::list<udp::recv*>::iterator it = obj->recv_queue.begin();
			if ((*it)->addr)
				*(*it)->addr = *addr;
			if ((*it)->buf)
			{
				if ((*it)->len > udp::routing_len)
					(*it)->len = udp::routing_len;
				if ((*it)->len)
					memcpy((*it)->buf, udp::routing_buf, (*it)->len);
			}
			else
				(*it)->len = udp::routing_len;
			event_task* target = reinterpret_cast<event_task*>(*it);
			obj->recv_queue.erase(it);
			__task_stand(target);
		}
	}
	udp::routing_len = 0;
}

udp::recv_by_seq::recv_by_seq(udp& handle, struct sockaddr* addr, void* buf, size_t& len, const void* seq, size_t seq_len)
	: handle(handle), addr(addr), buf(buf), len(len), seq(seq, seq_len)
{
	CHECK(seq != NULL && seq_len > 0);
	CHECK(this->handle.seqer != NULL);
}

udp::recv_by_seq::recv_by_seq(udp& handle, struct sockaddr* addr, void* buf, size_t& len, uint32 seq)
	: handle(handle), addr(addr), buf(buf), len(len), seq32(seq), seq(reinterpret_cast<const void*>(&this->seq32), sizeof(this->seq32))
{
	CHECK(this->handle.seqer != NULL);
}

void udp::recv_by_seq::run()
{
	if (!this->handle.receiving)
	{
		this->handle.receiving = 2;
		CHECK(uv_udp_recv_start(reinterpret_cast<uv_udp_t*>(this->handle.sock), udp::udp_alloc_cb1, udp::udp_recv_cb1) == 0);
	}
	this->pos = this->handle.seq_mapping.insert(std::pair<sequence, recv_by_seq*>(this->seq, this));
	(void)__task_yield(reinterpret_cast<event_task*>(this));
}

void udp::recv_by_seq::cancel()
{
	this->handle.seq_mapping.erase(this->pos);
}

udp::recv_by_seq::~recv_by_seq()
{
}

}
