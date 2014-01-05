#include "cocoflow-comm.h"

namespace ccf {

/***** tcp *****/

namespace tcp {

/***** tcp.listening *****/

listening::listening(int backlog)
	: sock(malloc(sizeof(uv_tcp_t))), backlog(backlog), accepting(false)
{
	CHECK(this->sock != NULL);
	CHECK(this->backlog > 0);
	CHECK(uv_tcp_init(loop(), reinterpret_cast<uv_tcp_t*>(this->sock)) == 0);
	reinterpret_cast<uv_tcp_t*>(this->sock)->data = this;
}

int listening::bind(const struct sockaddr_in& addr)
{
	CHECK(!this->accepting);
	return uv_tcp_bind(reinterpret_cast<uv_tcp_t*>(this->sock), addr);
}

int listening::bind(const struct sockaddr_in6& addr)
{
	CHECK(!this->accepting);
	return uv_tcp_bind6(reinterpret_cast<uv_tcp_t*>(this->sock), addr);
}

listening::~listening()
{
	uv_close(reinterpret_cast<uv_handle_t*>(this->sock), free_self_close_cb);
}

/***** tcp.accept *****/

void listening::tcp_accept_cb(uv_stream_t* server, int status)
{
	CHECK(status == 0);
	std::list<accept*>::iterator it = reinterpret_cast<listening*>(server->data)->accept_queue.begin();
	if (it != reinterpret_cast<listening*>(server->data)->accept_queue.end())
	{
		CHECK((*it)->conn.established == false);
		CHECK(uv_accept(server, reinterpret_cast<uv_stream_t*>((*it)->conn.sock)) == 0);
		(*it)->conn.established = true;
		(*it)->ret = success;
		event_task* target = reinterpret_cast<event_task*>(*it);
		reinterpret_cast<listening*>(server->data)->accept_queue.erase(it);
		__task_stand(target);
	}
	else
	{
		uv_tcp_t* tmp = reinterpret_cast<uv_tcp_t*>(malloc(sizeof(uv_tcp_t)));
		CHECK(uv_tcp_init(loop(), tmp) == 0);
		CHECK(uv_accept(server, reinterpret_cast<uv_stream_t*>(tmp)) == 0);
		uv_close(reinterpret_cast<uv_handle_t*>(tmp), free_self_close_cb);
	}
}

accept::accept(int& ret, listening& handle, connected& conn)
	: ret(ret), handle(handle), conn(conn)
{
	this->ret = unfinished;
}

void accept::run()
{
	this->pos = this->handle.accept_queue.insert(this->handle.accept_queue.end(), this);
	if (!this->handle.accepting)
	{
		int ret = uv_listen(reinterpret_cast<uv_stream_t*>(this->handle.sock), this->handle.backlog, listening::tcp_accept_cb);
		if (ret == -1 && uv_last_error(loop()).code == UV_EADDRINUSE)
		{
			this->ret = address_in_use;
			return;
		}
		CHECK(ret == 0);
		this->handle.accepting = true;
	}
	(void)__task_yield(reinterpret_cast<event_task*>(this));
}

void accept::cancel()
{
	this->handle.accept_queue.erase(this->pos);
}

accept::~accept()
{
}

/***** tcp.connected *****/

connected::connected()
	: sock(malloc(sizeof(uv_tcp_t))), established(false), broken(false), receiving(0), cur_alloc0(NULL), cur_alloc1(NULL),
	  buf1(NULL), size1(0), len1(0), buf2(NULL), size2(0), len2(0), header_len(0), packet_len(0),
	  lener(NULL), seqer(NULL), c_unrecv(0), c_failed(0), async_cancel1(NULL)
{
	CHECK(this->sock != NULL);
	CHECK(uv_tcp_init(loop(), reinterpret_cast<uv_tcp_t*>(this->sock)) == 0);
	reinterpret_cast<uv_tcp_t*>(this->sock)->data = this;
}

int connected::bind_inner(size_t min_len, size_t max_len, len_getter* lener)
{
	if (!min_len || !max_len || !lener || this->header_len || this->receiving)
		return -1;
	this->header_len = min_len;
	this->size2 = max_len;
	this->buf2 = reinterpret_cast<char*>(malloc(this->size2));
	CHECK(this->buf2 != NULL);
	this->lener = lener;
	this->receiving = 3;
	CHECK(uv_read_start(reinterpret_cast<uv_stream_t*>(this->sock), connected::tcp_alloc_cb2, connected::tcp_recv_cb2) == 0);
	return 0;
}

uint16 connected::peer_type()
{
	struct sockaddr_in6 peer;
	int len = sizeof(peer);
	CHECK(this->established == true);
	CHECK(uv_tcp_getpeername(reinterpret_cast<uv_tcp_t*>(this->sock), reinterpret_cast<struct sockaddr*>(&peer), &len) == 0);
	CHECK(len >= (int)sizeof(uint16));
	return peer.sin6_family;
}

struct sockaddr_in connected::peer_addr_ipv4()
{
	struct sockaddr_in6 peer;
	int len = sizeof(peer);
	CHECK(this->established == true);
	CHECK(uv_tcp_getpeername(reinterpret_cast<uv_tcp_t*>(this->sock), reinterpret_cast<struct sockaddr*>(&peer), &len) == 0);
	CHECK(len == (int)sizeof(struct sockaddr_in) && peer.sin6_family == AF_INET);
	return sockaddr_in_outof_sockaddr_in6(peer);
}

struct sockaddr_in6 connected::peer_addr_ipv6()
{
	struct sockaddr_in6 peer;
	int len = sizeof(peer);
	CHECK(this->established == true);
	CHECK(uv_tcp_getpeername(reinterpret_cast<uv_tcp_t*>(this->sock), reinterpret_cast<struct sockaddr*>(&peer), &len) == 0);
	CHECK(len == (int)sizeof(struct sockaddr_in6) && peer.sin6_family == AF_INET6);
	return peer;
}

unsigned long long connected::count_unrecv() const
{
	return this->c_unrecv;
}

unsigned long long connected::count_failed() const
{
	return this->c_failed;
}

const void* connected::internal_buffer(size_t& len)
{
	CHECK(this->receiving == 3);
	len = this->len2;
	return this->buf2;
}

typedef struct 
{
	uv_handle_t* handle;
	int err;
} drop_all_cb_data;

void connected::break_all_recv(uv_handle_t* handle, int err)
{
	connected* obj = reinterpret_cast<connected*>(handle->data);
	for (std::list<recv*>::iterator it = obj->recv_queue0.begin(); it != obj->recv_queue0.end(); )
	{
		(*it)->ret = err;
		event_task* target = *it;
		obj->recv_queue0.erase(it++);
		__task_stand(target);
		if (obj != reinterpret_cast<connected*>(handle->data))
			return;
	}
	for (std::list<recv_till*>::iterator it = obj->recv_queue1.begin(); it != obj->recv_queue1.end(); )
	{
		(*it)->ret = err;
		event_task* target = *it;
		obj->recv_queue1.erase(it++);
		__task_stand(target);
		if (obj != reinterpret_cast<connected*>(handle->data))
			return;
	}
	drop_all_cb_data data;
	data.handle = handle;
	data.err = err;
	obj->seqer->drop_all(connected::break_all_recv_by_seq, &data);
}

bool connected::break_all_recv_by_seq(void* obj, void* odata)
{
	recv_by_seq_if* rbs = reinterpret_cast<recv_by_seq_if*>(obj);
	drop_all_cb_data* data = reinterpret_cast<drop_all_cb_data*>(odata);
	void* record = data->handle->data;
	rbs->ret = data->err;
	__task_stand(rbs);
	if (record != data->handle->data)
		return true;
	else
		return false;
}

connected::~connected()
{
	if (this->receiving)
		CHECK(uv_read_stop(reinterpret_cast<uv_stream_t*>(this->sock)) == 0);
	if (this->async_cancel1)
		uv_close(reinterpret_cast<uv_handle_t*>(this->async_cancel1), free_self_close_cb);
	uv_close(reinterpret_cast<uv_handle_t*>(this->sock), free_self_close_cb);
	reinterpret_cast<uv_tcp_t*>(this->sock)->data = NULL;
	if (this->buf1)
		free(this->buf1);
	if (this->buf2)
		free(this->buf2);
	if (this->seqer)
		delete this->seqer;
}

/***** tcp.connect *****/

void connected::tcp_connect_cb(uv_connect_t* req, int status)
{
	if (!req->data || (status == -1 && uv_last_error(loop()).code == UV_ECANCELED))
		; //Canceled or Canceled by sock close
	else
	{
		if (status == 0)
		{
			reinterpret_cast<connect*>(req->data)->ret = success;
			CHECK(reinterpret_cast<connect*>(req->data)->handle.established == false);
			reinterpret_cast<connect*>(req->data)->handle.established = true;
		}
		else
		{
			reinterpret_cast<connect*>(req->data)->handle.broken = true;
			reinterpret_cast<connect*>(req->data)->ret = failure;
		}
		__task_stand(reinterpret_cast<event_task*>(req->data));
	}
	free(req);
}

connect::connect(int& ret, connected& handle, const struct sockaddr_in& addr)
	: req(NULL), ret(ret), handle(handle), addr(sockaddr_in_into_sockaddr_in6(addr))
{
	CHECK(this->addr.sin6_family == AF_INET);
	this->ret = unfinished;
}

connect::connect(int& ret, connected& handle, const struct sockaddr_in6& addr)
	: req(NULL), ret(ret), handle(handle), addr(addr)
{
	CHECK(this->addr.sin6_family == AF_INET6);
	this->ret = unfinished;
}

void connect::run()
{
	CHECK(this->handle.broken == false);
	this->req = malloc(sizeof(uv_connect_t));
	CHECK(this->req != NULL);
	if (this->addr.sin6_family == AF_INET)
		CHECK(uv_tcp_connect(reinterpret_cast<uv_connect_t*>(this->req), reinterpret_cast<uv_tcp_t*>(this->handle.sock), *reinterpret_cast<const struct sockaddr_in*>(&this->addr), connected::tcp_connect_cb) == 0);
	else //AF_INET6
		CHECK(uv_tcp_connect6(reinterpret_cast<uv_connect_t*>(this->req), reinterpret_cast<uv_tcp_t*>(this->handle.sock), this->addr, connected::tcp_connect_cb) == 0);
	reinterpret_cast<uv_connect_t*>(this->req)->data = this;
	(void)__task_yield(reinterpret_cast<event_task*>(this));
}

void connect::cancel()
{
	this->handle.broken = true;
	reinterpret_cast<uv_connect_t*>(this->req)->data = NULL;
}

connect::~connect()
{
}

/***** tcp.send *****/

void connected::tcp_send_cb(uv_write_t* req, int status)
{
	if (status == -1 && uv_last_error(loop()).code == UV_ECANCELED)
		return; //Canceled by sock close
	if (status == 0)
		reinterpret_cast<send*>(req->data)->ret = success;
	else
	{
		reinterpret_cast<send*>(req->data)->handle.broken = true;
		reinterpret_cast<send*>(req->data)->ret = failure;
	}
	__task_stand(reinterpret_cast<event_task*>(req->data));
}

send::send(int& ret, connected& handle, const void* buf0, size_t len0)
	: ret(ret), handle(handle), num(1)
{
	CHECK(buf0 != NULL && len0 > 0);
	CHECK(this->handle.established == true);
	this->ret = unfinished;
	this->buf[0] = uv_buf_init(reinterpret_cast<char*>(const_cast<void*>(buf0)), static_cast<unsigned int>(len0));
	this->uninterruptable();
}

send::send(int& ret, connected& handle, const void* buf0, size_t len0, const void* buf1, size_t len1)
	: ret(ret), handle(handle), num(2)
{
	CHECK(buf0 != NULL && len0 > 0 && buf1 != NULL && len1 > 0);
	CHECK(this->handle.established == true);
	this->ret = unfinished;
	this->buf[0] = uv_buf_init(reinterpret_cast<char*>(const_cast<void*>(buf0)), static_cast<unsigned int>(len0));
	this->buf[1] = uv_buf_init(reinterpret_cast<char*>(const_cast<void*>(buf1)), static_cast<unsigned int>(len1));
	this->uninterruptable();
}

send::send(int& ret, connected& handle, const void* buf0, size_t len0, const void* buf1, size_t len1, const void* buf2, size_t len2)
	: ret(ret), handle(handle), num(3)
{
	CHECK(buf0 != NULL && len0 > 0 && buf1 != NULL && len1 > 0 && buf2 != NULL && len2 > 0);
	CHECK(this->handle.established == true);
	this->ret = unfinished;
	this->buf[0] = uv_buf_init(reinterpret_cast<char*>(const_cast<void*>(buf0)), static_cast<unsigned int>(len0));
	this->buf[1] = uv_buf_init(reinterpret_cast<char*>(const_cast<void*>(buf1)), static_cast<unsigned int>(len1));
	this->buf[2] = uv_buf_init(reinterpret_cast<char*>(const_cast<void*>(buf2)), static_cast<unsigned int>(len2));
	this->uninterruptable();
}

send::send(int& ret, connected& handle, const void* buf0, size_t len0, const void* buf1, size_t len1, const void* buf2, size_t len2, const void* buf3, size_t len3)
	: ret(ret), handle(handle), num(4)
{
	CHECK(buf0 != NULL && len0 > 0 && buf1 != NULL && len1 > 0 && buf2 != NULL && len2 > 0 && buf3 != NULL && len3 > 0);
	CHECK(this->handle.established == true);
	this->ret = unfinished;
	this->buf[0] = uv_buf_init(reinterpret_cast<char*>(const_cast<void*>(buf0)), static_cast<unsigned int>(len0));
	this->buf[1] = uv_buf_init(reinterpret_cast<char*>(const_cast<void*>(buf1)), static_cast<unsigned int>(len1));
	this->buf[2] = uv_buf_init(reinterpret_cast<char*>(const_cast<void*>(buf2)), static_cast<unsigned int>(len2));
	this->buf[3] = uv_buf_init(reinterpret_cast<char*>(const_cast<void*>(buf3)), static_cast<unsigned int>(len3));
	this->uninterruptable();
}

void send::run()
{
	CHECK(this->handle.broken == false);
	uv_write_t req;
	CHECK(uv_write(&req, reinterpret_cast<uv_stream_t*>(this->handle.sock), this->buf, this->num, connected::tcp_send_cb) == 0);
	req.data = this;
	(void)__task_yield(reinterpret_cast<event_task*>(this));
}

void send::cancel()
{
	CHECK(0);
}

send::~send()
{
}

/***** tcp.recv *****/

uv_buf_t connected::tcp_alloc_cb0(uv_handle_t* handle, size_t suggested_size)
{
	connected* obj = reinterpret_cast<connected*>(handle->data);
	CHECK(obj->cur_alloc0 == NULL);
	std::list<recv*>::iterator it = obj->recv_queue0.begin();
	CHECK(it != obj->recv_queue0.end());
	obj->cur_alloc0 = *it;
	obj->recv_queue0.erase(it);
	CHECK(obj->cur_alloc0->buf != NULL && obj->cur_alloc0->len > 0);
	return uv_buf_init(reinterpret_cast<char*>(obj->cur_alloc0->buf), static_cast<unsigned int>(obj->cur_alloc0->len));
}

void connected::tcp_recv_cb0(uv_stream_t* handle, ssize_t nread, uv_buf_t buf)
{
	connected* obj = reinterpret_cast<connected*>(handle->data);
	CHECK(obj->cur_alloc0 != NULL);
	if (nread < 0)
	{
		obj->broken = true;
		obj->cur_alloc0->ret = failure;
		event_task* target = obj->cur_alloc0;
		obj->cur_alloc0 = NULL;
		__task_stand(target);
		if (obj == reinterpret_cast<connected*>(handle->data))
			connected::break_all_recv(reinterpret_cast<uv_handle_t*>(handle), failure);
		return;
	}
	if (nread == 0)
	{
		obj->cur_alloc0->pos = obj->recv_queue0.insert(obj->recv_queue0.begin(), obj->cur_alloc0);
		obj->cur_alloc0 = NULL;
	}
	else
	{
		obj->cur_alloc0->len = nread;
		obj->cur_alloc0->ret = success;
		event_task* target = obj->cur_alloc0;
		obj->cur_alloc0 = NULL;
		__task_stand(target);
		if (obj == reinterpret_cast<connected*>(handle->data)) //Check valid
		{
			if (obj->recv_queue0.empty())
			{
				CHECK(uv_read_stop(reinterpret_cast<uv_stream_t*>(obj->sock)) == 0);
				obj->receiving = 0;
			}
		}
	}
}

recv::recv(int& ret, connected& handle, void* buf, size_t& len)
	: ret(ret), handle(handle), buf(buf), len(len)
{
	this->ret = unfinished;
}

void recv::run()
{
	CHECK(this->handle.broken == false);
	CHECK(this->handle.receiving != 2);
	if (!this->handle.receiving)
	{
		this->handle.receiving = 1;
		CHECK(uv_read_start(reinterpret_cast<uv_stream_t*>(this->handle.sock), connected::tcp_alloc_cb0, connected::tcp_recv_cb0) == 0);
	}
	this->pos = this->handle.recv_queue0.insert(this->handle.recv_queue0.end(), this);
	(void)__task_yield(reinterpret_cast<event_task*>(this));
}

void recv::cancel()
{
	this->handle.recv_queue0.erase(this->pos);
	if (this->handle.receiving == 1 && this->handle.recv_queue0.empty())
	{
		CHECK(uv_read_stop(reinterpret_cast<uv_stream_t*>(this->handle.sock)) == 0);
		this->handle.receiving = 0;
	}
}

recv::~recv()
{
}

/***** tcp.recv_till *****/

/* success:(pos of buf1 buf2 first match + len2), failure:NULL */
static const void *bufbuf(const void *buf1, size_t len1, const void *buf2, size_t len2)
{
	if (!buf1 || !len1 || len1 < len2)
		return NULL;
	if (!buf2 || !len2)
		return buf1;
	const void *ret = NULL;
	for (size_t i=0; i<len1-len2+1; i++)
	{
		ret = (const char *)buf1 + i + len2;
		for (size_t j=0; j<len2; j++)
		{
			if (((const unsigned char *)buf1)[i+j] != ((const unsigned char *)buf2)[j])
			{
				ret = NULL;
				break;
			}
		}
		if (ret)
			break;
	}
	return ret;
}

uv_buf_t connected::tcp_alloc_cb1(uv_handle_t* handle, size_t suggested_size)
{
	connected* obj = reinterpret_cast<connected*>(handle->data);
	CHECK(obj->cur_alloc1 == NULL);
	std::list<recv_till*>::iterator it = obj->recv_queue1.begin();
	CHECK(it != obj->recv_queue1.end());
	obj->cur_alloc1 = *it;
	obj->recv_queue1.erase(it);
	if (!obj->cur_alloc1->pattern)
	{
		CHECK(obj->len1 == 0);
		return uv_buf_init(reinterpret_cast<char*>(obj->cur_alloc1->cur), static_cast<unsigned int>(obj->cur_alloc1->left));
	}
	else
	{
		if (!obj->buf1)
		{
			obj->size1 = 1024;
			obj->buf1 = reinterpret_cast<char*>(malloc(obj->size1));
			CHECK(obj->buf1 != NULL);
		}
		return uv_buf_init(reinterpret_cast<char*>(obj->buf1 + obj->len1), static_cast<unsigned int>(obj->size1 - obj->len1));
	}
}

void connected::tcp_recv_cb1(uv_stream_t* handle, ssize_t nread, uv_buf_t buf)
{
	connected* obj = reinterpret_cast<connected*>(handle->data);
	CHECK(obj->cur_alloc1 != NULL);
	if (nread < 0)
	{
		obj->broken = true;
		obj->cur_alloc1->ret = failure;
		event_task* target = obj->cur_alloc1;
		obj->cur_alloc1 = NULL;
		__task_stand(target);
		if (obj == reinterpret_cast<connected*>(handle->data))
			connected::break_all_recv(reinterpret_cast<uv_handle_t*>(handle), failure);
		return;
	}
	if (nread == 0)
	{
		obj->cur_alloc1->pos = obj->recv_queue1.insert(obj->recv_queue1.begin(), obj->cur_alloc1);
		obj->cur_alloc1 = NULL;
	}
	else
	{
		if (!obj->cur_alloc1->pattern)
		{
			CHECK(nread <= static_cast<ssize_t>(obj->cur_alloc1->left));
			if (nread == static_cast<ssize_t>(obj->cur_alloc1->left))
			{
				obj->cur_alloc1->ret = success;
				event_task* target = obj->cur_alloc1;
				obj->cur_alloc1 = NULL;
				__task_stand(target);
			}
			else
			{
				obj->cur_alloc1->cur = reinterpret_cast<char*>(obj->cur_alloc1->cur) + nread;
				obj->cur_alloc1->left -= nread;
				obj->cur_alloc1->pos = obj->recv_queue1.insert(obj->recv_queue1.begin(), obj->cur_alloc1);
				obj->cur_alloc1 = NULL;
			}
		}
		else
		{
			CHECK(nread <= static_cast<ssize_t>(obj->size1 - obj->len1));
			obj->len1 += nread;
			if (obj->cur_alloc1->remaining())
			{
				obj->cur_alloc1->ret = success;
				event_task* target = obj->cur_alloc1;
				obj->cur_alloc1 = NULL;
				__task_stand(target);
			}
			else
			{
				obj->cur_alloc1->pos = obj->recv_queue1.insert(obj->recv_queue1.begin(), obj->cur_alloc1);
				obj->cur_alloc1 = NULL;
			}
		}
	}
	if (nread > 0 && obj == reinterpret_cast<connected*>(handle->data)) //Check valid
		connected::check_remaining(reinterpret_cast<uv_handle_t*>(handle));
}

void connected::tcp_fallback_cb1(uv_async_t* async, int status)
{
	CHECK(status == 0);
	connected::check_remaining(reinterpret_cast<uv_handle_t*>(async));
}

recv_till::recv_till(int& ret, connected& handle, void* buf, size_t& len)
	: ret(ret), handle(handle), buf(buf), len(len), pattern(NULL), pattern_len(0), cur(buf), left(len)
{
	CHECK(buf != NULL && len > 0);
	this->ret = unfinished;
}

recv_till::recv_till(int& ret, connected& handle, void* buf, size_t& len, const void* pattern, size_t pattern_len)
	: ret(ret), handle(handle), buf(buf), len(len), pattern(pattern), pattern_len(pattern_len), cur(buf), left(len)
{
	CHECK(buf != NULL && len > 0 && pattern != NULL && pattern_len > 0);
	this->ret = unfinished;
}

void recv_till::run()
{
	CHECK(this->handle.broken == false);
	if (this->handle.len1 && this->handle.recv_queue1.empty() && this->remaining())
	{
		this->ret = success;
		return;
	}
	CHECK(this->handle.receiving == 0 || this->handle.receiving == 2);
	if (!this->handle.receiving)
	{
		this->handle.receiving = 2;
		CHECK(uv_read_start(reinterpret_cast<uv_stream_t*>(this->handle.sock), connected::tcp_alloc_cb1, connected::tcp_recv_cb1) == 0);
		this->handle.async_cancel1 = malloc(sizeof(uv_async_t));
		CHECK(this->handle.async_cancel1 != NULL);
		CHECK(uv_async_init(loop(), reinterpret_cast<uv_async_t*>(this->handle.async_cancel1), connected::tcp_fallback_cb1) == 0);
		reinterpret_cast<uv_async_t*>(this->handle.async_cancel1)->data = &this->handle;
	}
	this->pos = this->handle.recv_queue1.insert(this->handle.recv_queue1.end(), this);
	(void)__task_yield(reinterpret_cast<event_task*>(this));
}

void recv_till::cancel()
{
	this->handle.recv_queue1.erase(this->pos);
	if (this->cur != this->buf)
	{
		size_t fallback = reinterpret_cast<char*>(this->cur) - reinterpret_cast<char*>(this->buf);
		if (fallback > this->handle.size1 - this->handle.len1)
		{
			void* old = this->handle.buf1;
			this->handle.size1 += this->handle.len1 + fallback;
			this->handle.buf1 = reinterpret_cast<char*>(malloc(this->handle.size1));
			CHECK(this->handle.buf1 != NULL);
			memcpy(this->handle.buf1, old, this->handle.len1);
			free(old);
		}
		memcpy(this->handle.buf1 + this->handle.len1, this->buf, fallback);
		this->handle.len1 += fallback;
	}
	CHECK(uv_async_send(reinterpret_cast<uv_async_t*>(this->handle.async_cancel1)) == 0);
}

bool recv_till::remaining()
{
	CHECK(this->handle.len1 > 0);
	if (!this->pattern)
	{
		if (this->handle.len1 >= this->left)
		{
____remaining_fill:
			memcpy(this->cur, this->handle.buf1, this->left);
			this->handle.len1 -= this->left;
			if (this->handle.len1)
				memmove(this->handle.buf1, this->handle.buf1 + this->left, this->handle.len1);
			return true;
		}
		else
		{
			memcpy(this->cur, this->handle.buf1, this->handle.len1);
			this->left -= this->handle.len1;
			this->cur = reinterpret_cast<char*>(this->cur) + this->handle.len1;
			this->handle.len1 = 0;
			return false;
		}
	}
	else
	{
		if (this->handle.len1 < this->pattern_len)
			return false;
		void *ret = const_cast<void*>(bufbuf(this->handle.buf1, this->handle.len1, this->pattern, this->pattern_len));
		if (ret)
		{
			size_t copy = reinterpret_cast<char*>(ret) - reinterpret_cast<char*>(this->handle.buf1);
			if (copy >= this->left)
				goto ____remaining_fill;
			memcpy(this->cur, this->handle.buf1, copy);
			this->handle.len1 -= copy;
			if (this->handle.len1)
				memmove(this->handle.buf1, this->handle.buf1 + copy, this->handle.len1);
			this->len = reinterpret_cast<char*>(this->cur) - reinterpret_cast<char*>(this->buf) + copy;
			return true;
		}
		else
		{
			size_t copy = this->handle.len1 - this->pattern_len + 1;
			if (copy >= this->left)
				goto ____remaining_fill;
			memcpy(this->cur, this->handle.buf1, copy);
			this->left -= copy;
			this->cur = reinterpret_cast<char*>(this->cur) + copy;
			this->handle.len1 -= copy;
			if (this->handle.len1)
				memmove(this->handle.buf1, this->handle.buf1 + copy, this->handle.len1);
			return false;
		}
	}
}

recv_till::~recv_till()
{
}

void connected::check_remaining(uv_handle_t* handle)
{
	connected* obj = reinterpret_cast<connected*>(handle->data);
	while (obj->len1 && obj->recv_queue1.size())
	{
		std::list<recv_till*>::iterator it = obj->recv_queue1.begin();
		if ((*it)->remaining())
		{
			event_task* target = reinterpret_cast<event_task*>(*it);
			(*it)->ret = success;
			obj->recv_queue1.erase(it);
			__task_stand(target);
			if (obj != reinterpret_cast<connected*>(handle->data))
				return;
		}
		else
			break;
	}
	if (obj->recv_queue1.empty())
	{
		CHECK(uv_read_stop(reinterpret_cast<uv_stream_t*>(obj->sock)) == 0);
		obj->receiving = 0;
		uv_close(reinterpret_cast<uv_handle_t*>(obj->async_cancel1), free_self_close_cb);
		obj->async_cancel1 = NULL;
	}
}

/***** tcp.recv_by_seq *****/

uv_buf_t connected::tcp_alloc_cb2(uv_handle_t* handle, size_t suggested_size)
{
	connected* obj = reinterpret_cast<connected*>(handle->data);
	if (obj->len2 < obj->header_len)
		return uv_buf_init(obj->buf2 + obj->len2, obj->header_len - obj->len2);
	else
		return uv_buf_init(obj->buf2 + obj->len2, obj->packet_len - obj->len2);
}

void connected::tcp_recv_cb2(uv_stream_t* handle, ssize_t nread, uv_buf_t buf)
{
	if (nread == 0)
		return;
	if (nread < 0)
	{
		reinterpret_cast<connected*>(handle->data)->broken = true;
		connected::break_all_recv(reinterpret_cast<uv_handle_t*>(handle), failure);
		return;
	}
	connected* obj = reinterpret_cast<connected*>(handle->data);
	obj->len2 += nread;
	if (!obj->packet_len && obj->len2 == obj->header_len)
	{
		obj->packet_len = obj->lener(obj->buf2, obj->len2);
		if (obj->packet_len < obj->header_len)
		{
			obj->broken = true;
			connected::break_all_recv(reinterpret_cast<uv_handle_t*>(handle), packet_length_too_short);
			return;
		}
		if (obj->packet_len > obj->size2)
		{
			obj->broken = true;
			connected::break_all_recv(reinterpret_cast<uv_handle_t*>(handle), packet_length_too_long);
			return;
		}
	}
	if (obj->packet_len && obj->len2 == obj->packet_len)
	{
		recv_by_seq_if* rbs;
		int ret = obj->seqer->unwrap(obj->buf2, obj->len2, reinterpret_cast<void**>(&rbs));
		if (ret >= 0)
		{
			if (rbs != NULL)
			{
				if (rbs->buf)
				{
					if (rbs->len > obj->len2)
						rbs->len = obj->len2;
					if (rbs->len)
						memcpy(rbs->buf, obj->buf2, rbs->len);
				}
				else
					rbs->len = obj->len2;
				rbs->ret = success;
				__task_stand(reinterpret_cast<event_task*>(rbs));
			}
			else
			{
				if (obj->recv_queue0.empty())
				{
					obj->c_unrecv ++;
					obj->seqer->call_unrecv(obj->buf2, obj->len2);
				}
				else
					goto ____tcp_recv_cb_rqne;
			}
		}
		else
		{
			if (obj->recv_queue0.empty())
			{
				obj->c_failed ++;
				obj->seqer->call_failed(obj->buf2, obj->len2, ret);
			}
			else
			{
____tcp_recv_cb_rqne:
				std::list<recv*>::iterator it = obj->recv_queue0.begin();
				if ((*it)->buf)
				{
					if ((*it)->len > obj->len2)
						(*it)->len = obj->len2;
					if ((*it)->len)
						memcpy((*it)->buf, obj->buf2, (*it)->len);
				}
				else
					(*it)->len = obj->len2;
				event_task* target = reinterpret_cast<event_task*>(*it);
				(*it)->ret = success;
				obj->recv_queue0.erase(it);
				__task_stand(target);
			}
		}
		if (obj == reinterpret_cast<connected*>(handle->data)) //Check valid
		{
			obj->len2 = 0;
			obj->packet_len = 0;
		}
	}
}

recv_by_seq_if::recv_by_seq_if(int& ret, connected& handle, void* buf, size_t& len)
	: ret(ret), handle(handle), buf(buf), len(len)
{
	this->ret = unfinished;
}

void recv_by_seq_if::run_part_0()
{
	CHECK(this->handle.broken == false);
	CHECK(this->handle.receiving == 3);
}

void recv_by_seq_if::run_part_1()
{
	(void)__task_yield(reinterpret_cast<event_task*>(this));
}

recv_by_seq_if::~recv_by_seq_if()
{
}

}

}
