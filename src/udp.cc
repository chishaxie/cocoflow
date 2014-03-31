#include "cocoflow-comm.h"

namespace ccf {

/***** udp *****/

enum {
	udp_receiving_ready = 0,
	udp_receiving_recv,
	udp_receiving_recv_by_seq,
	udp_receiving_ignore
};

udp::udp()
	: sock(malloc(sizeof(uv_udp_t))), receiving(udp_receiving_ready), cur_alloc(NULL),
	  seqer(NULL), ignored(NULL), c_unrecv(0), c_failed(0), c_ignored(0)
{
	CHECK(this->sock != NULL);
	CHECK(uv_udp_init(loop(), reinterpret_cast<uv_udp_t*>(this->sock)) == 0);
	reinterpret_cast<uv_udp_t*>(this->sock)->data = this;
}

int udp::bind(const struct sockaddr_in& addr)
{
	CHECK(this->receiving == udp_receiving_ready);
	return uv_udp_bind(reinterpret_cast<uv_udp_t*>(this->sock), addr, 0);
}

int udp::bind(const struct sockaddr_in6& addr, bool ipv6_only)
{
	CHECK(this->receiving == udp_receiving_ready);
	return uv_udp_bind6(reinterpret_cast<uv_udp_t*>(this->sock), addr, ipv6_only? UV_UDP_IPV6ONLY: 0);
}

void udp::ignore_recv(pkg_ignored* ignored)
{
	CHECK(this->receiving == udp_receiving_ready);
	this->ignored = ignored;
	this->receiving = udp_receiving_ignore;
	CHECK(uv_udp_recv_start(reinterpret_cast<uv_udp_t*>(this->sock), udp::udp_alloc_cb2, udp::udp_recv_cb2) == 0);
}

unsigned long long udp::count_unrecv() const
{
	return this->c_unrecv;
}

unsigned long long udp::count_failed() const
{
	return this->c_failed;
}

unsigned long long udp::count_ignored() const
{
	return this->c_ignored;
}

udp::~udp()
{
	if (this->receiving != udp_receiving_ready)
		CHECK(uv_udp_recv_stop(reinterpret_cast<uv_udp_t*>(this->sock)) == 0);
	uv_close(reinterpret_cast<uv_handle_t*>(this->sock), free_self_close_cb);
	reinterpret_cast<uv_udp_t*>(this->sock)->data = NULL;
	if (this->seqer)
		delete this->seqer;
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
	: handle(handle), addr(sockaddr_in_into_sockaddr_in6(addr)),
	  buf(uv_buf_init(reinterpret_cast<char*>(const_cast<void*>(buf)), static_cast<unsigned int>(len)))
{
	CHECK(this->addr.sin6_family == AF_INET);
	CHECK(buf != NULL && len > 0);
	this->uninterruptable();
}

udp::send::send(udp& handle, const struct sockaddr_in6& addr, const void* buf, size_t len)
	: handle(handle), addr(addr),
	  buf(uv_buf_init(reinterpret_cast<char*>(const_cast<void*>(buf)), static_cast<unsigned int>(len)))
{
	CHECK(this->addr.sin6_family == AF_INET6);
	CHECK(buf != NULL && len > 0);
	this->uninterruptable();
}

void udp::send::run()
{
	uv_udp_send_t req;
	if (this->addr.sin6_family == AF_INET)
		CHECK(uv_udp_send(&req, reinterpret_cast<uv_udp_t*>(this->handle.sock), &this->buf, 1, *reinterpret_cast<const struct sockaddr_in*>(&this->addr), udp_send_cb) == 0);
	else //AF_INET6
		CHECK(uv_udp_send6(&req, reinterpret_cast<uv_udp_t*>(this->handle.sock), &this->buf, 1, this->addr, udp_send_cb) == 0);
	req.data = this;
	(void)__task_yield(this);
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
		if (addr->sa_family == AF_INET)
			obj->cur_alloc->addr = sockaddr_in_into_sockaddr_in6(*reinterpret_cast<struct sockaddr_in*>(addr));
		else if (addr->sa_family == AF_INET6)
			obj->cur_alloc->addr = *reinterpret_cast<struct sockaddr_in6*>(addr);
		else
			obj->cur_alloc->addr.sin6_family = addr->sa_family;
		obj->cur_alloc->len = nread;
		if (!obj->cur_alloc->buf || !obj->cur_alloc->len)
			udp::routing_len = nread; //data in udp::routing_buf
		event_task* target = obj->cur_alloc;
		obj->cur_alloc = NULL;
		__task_stand(target);
		udp::routing_len = 0;
		if (obj == reinterpret_cast<udp*>(handle->data)) //Check valid
		{
			if (obj->recv_queue.empty())
			{
				CHECK(uv_udp_recv_stop(reinterpret_cast<uv_udp_t*>(obj->sock)) == 0);
				obj->receiving = udp_receiving_ready;
			}
		}
	}
}

udp::recv::recv(udp& handle, void* buf, size_t& len)
	: handle(handle), buf(buf), len(len)
{
	this->addr.sin6_family = 0xffff;
}

uint16 udp::recv::peer_type()
{
	return this->addr.sin6_family;
}

struct sockaddr_in udp::recv::peer_addr_ipv4()
{
	CHECK(this->addr.sin6_family == AF_INET);
	return sockaddr_in_outof_sockaddr_in6(this->addr);
}

struct sockaddr_in6 udp::recv::peer_addr_ipv6()
{
	CHECK(this->addr.sin6_family == AF_INET6);
	return this->addr;
}

void udp::recv::run()
{
	CHECK(this->handle.receiving != udp_receiving_ignore);
	if (this->handle.receiving == udp_receiving_ready)
	{
		if (!this->handle.seqer)
		{
			this->handle.receiving = udp_receiving_recv;
			CHECK(uv_udp_recv_start(reinterpret_cast<uv_udp_t*>(this->handle.sock), udp::udp_alloc_cb0, udp::udp_recv_cb0) == 0);
		}
		else
		{
			this->handle.receiving = udp_receiving_recv_by_seq;
			CHECK(uv_udp_recv_start(reinterpret_cast<uv_udp_t*>(this->handle.sock), udp::udp_alloc_cb1, udp::udp_recv_cb1) == 0);
		}
	}
	this->pos = this->handle.recv_queue.insert(this->handle.recv_queue.end(), this);
	(void)__task_yield(this);
}

void udp::recv::cancel()
{
	this->handle.recv_queue.erase(this->pos);
	if (this->handle.receiving == udp_receiving_recv && this->handle.recv_queue.empty())
	{
		CHECK(uv_udp_recv_stop(reinterpret_cast<uv_udp_t*>(this->handle.sock)) == 0);
		this->handle.receiving = udp_receiving_ready;
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
	udp::recv_by_seq_if* rbs;
	int ret = obj->seqer->unwrap(udp::routing_buf, nread, reinterpret_cast<void**>(&rbs));
	if (ret >= 0)
	{
		if (rbs != NULL)
		{
			if (addr->sa_family == AF_INET)
				rbs->addr = sockaddr_in_into_sockaddr_in6(*reinterpret_cast<struct sockaddr_in*>(addr));
			else if (addr->sa_family == AF_INET6)
				rbs->addr = *reinterpret_cast<struct sockaddr_in6*>(addr);
			else
				rbs->addr.sin6_family = addr->sa_family;
			if (rbs->buf)
			{
				if (rbs->len > udp::routing_len)
					rbs->len = udp::routing_len;
				if (rbs->len)
					memcpy(rbs->buf, udp::routing_buf, rbs->len);
			}
			else
				rbs->len = udp::routing_len;
			__task_stand(rbs);
		}
		else
		{
			if (obj->recv_queue.empty())
			{
				obj->c_unrecv ++;
				obj->seqer->call_unrecv(udp::routing_buf, udp::routing_len);
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
			obj->seqer->call_failed(udp::routing_buf, udp::routing_len, ret);
		}
		else
		{
____udp_recv_cb_rqne:
			std::list<udp::recv*>::iterator it = obj->recv_queue.begin();
			udp::recv* r = *it;
			obj->recv_queue.erase(it);
			if (addr->sa_family == AF_INET)
				r->addr = sockaddr_in_into_sockaddr_in6(*reinterpret_cast<struct sockaddr_in*>(addr));
			else if (addr->sa_family == AF_INET6)
				r->addr = *reinterpret_cast<struct sockaddr_in6*>(addr);
			else
				r->addr.sin6_family = addr->sa_family;
			if (r->buf)
			{
				if (r->len > udp::routing_len)
					r->len = udp::routing_len;
				if (r->len)
					memcpy(r->buf, udp::routing_buf, r->len);
			}
			else
				r->len = udp::routing_len;
			__task_stand(r);
		}
	}
	udp::routing_len = 0;
}

udp::recv_by_seq_if::recv_by_seq_if(udp& handle, void* buf, size_t& len)
	: handle(handle), buf(buf), len(len)
{
	CHECK(this->handle.seqer != NULL);
	this->addr.sin6_family = 0xffff;
}

uint16 udp::recv_by_seq_if::peer_type()
{
	return this->addr.sin6_family;
}

struct sockaddr_in udp::recv_by_seq_if::peer_addr_ipv4()
{
	CHECK(this->addr.sin6_family == AF_INET);
	return sockaddr_in_outof_sockaddr_in6(this->addr);
}

struct sockaddr_in6 udp::recv_by_seq_if::peer_addr_ipv6()
{
	CHECK(this->addr.sin6_family == AF_INET6);
	return this->addr;
}

void udp::recv_by_seq_if::run_part_0()
{
	CHECK(this->handle.receiving != udp_receiving_ignore);
	if (this->handle.receiving == udp_receiving_ready)
	{
		this->handle.receiving = udp_receiving_recv_by_seq;
		CHECK(uv_udp_recv_start(reinterpret_cast<uv_udp_t*>(this->handle.sock), udp::udp_alloc_cb1, udp::udp_recv_cb1) == 0);
	}
}

void udp::recv_by_seq_if::run_part_1()
{
	(void)__task_yield(this);
}

udp::recv_by_seq_if::~recv_by_seq_if()
{
}

/***** udp.ignore *****/

uv_buf_t udp::udp_alloc_cb2(uv_handle_t* handle, size_t suggested_size)
{
	return uv_buf_init(udp::routing_buf, sizeof(udp::routing_buf));
}

void udp::udp_recv_cb2(uv_udp_t* handle, ssize_t nread, uv_buf_t buf, struct sockaddr* addr, unsigned flags)
{
	CHECK(nread >= 0);
	if (nread == 0)
		return;
	udp* obj = reinterpret_cast<udp*>(handle->data);
	udp::routing_len = nread;
	obj->c_ignored ++;
	if (obj->ignored)
		obj->ignored(udp::routing_buf, udp::routing_len, addr);
}

}
