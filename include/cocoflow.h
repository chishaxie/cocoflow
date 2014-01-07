#ifndef __COCOFLOW_H__
#define __COCOFLOW_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_MSC_VER) || _MSC_VER >= 1600
# include <stdint.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
# include <winsock2.h>
# include <ws2tcpip.h>
	#if !defined(_SSIZE_T_) && !defined(_SSIZE_T_DEFINED)
		typedef intptr_t ssize_t;
		#define _SSIZE_T_
		#define _SSIZE_T_DEFINED
	#endif
#else
# include <netinet/in.h>
#endif

#include <list>
#include <map>
#include <set>
#include <string>

/* Simplified uv.h */
extern "C" {
#ifndef __disable_simplify_uv_h__
	#if defined(__SVR4) && !defined(__unix__)
	# define __unix__
	#endif
	#if defined(__unix__) || defined(__POSIX__) || defined(__APPLE__) || defined(_AIX)
		typedef struct {
		  char* base;
		  size_t len;
		} uv_buf_t;
	#else
		typedef struct uv_buf_t {
		  ULONG len;
		  char* base;
		} uv_buf_t;
	#endif
	typedef struct uv_handle_s uv_handle_t;
	typedef struct uv_stream_s uv_stream_t;
	typedef struct uv_tcp_s uv_tcp_t;
	typedef struct uv_udp_s uv_udp_t;
	typedef struct uv_async_s uv_async_t;
	typedef struct uv_write_s uv_write_t;
	typedef struct uv_connect_s uv_connect_t;
#endif
}

namespace ccf {

#if !defined(_MSC_VER) || _MSC_VER >= 1600
	typedef uint16_t uint16;
	typedef uint32_t uint32;
	typedef uint64_t uint64;
#else
	typedef unsigned __int16 uint16;
	typedef unsigned __int32 uint32;
	typedef unsigned __int64 uint64;
#endif

static const size_t PAGE_SIZE = 4096;

template<uint32 UserPages, uint32 ProtectPages = 1>
class task;

typedef task<4> event_task;
typedef task<63> user_task;

#define _task_tpl task<UserPages, ProtectPages>

template<uint32 UserPages, uint32 ProtectPages>
inline int await(_task_tpl& target);

template<uint32 UserPages, uint32 ProtectPages>
inline int start(_task_tpl* target);

template<uint32 UserPages, uint32 ProtectPages>
void cocoflow(_task_tpl& top);

enum task_status
{
	bug = 0,
	ready,
	running,
	completed,
	canceled,
	limited,
	child_unready
};

template<uint32 UserPages, uint32 ProtectPages>
class task
{
public:
	inline task();
	virtual ~task();
	inline task_status status() const;
	inline uint32 unique_id() const;
	inline void uninterruptable();
	static void init(uint32 max_task_num);
protected:
	virtual void run() = 0;
	virtual void cancel();
private:
	task(const task&);
	task& operator=(const task&);
	event_task* reuse;
	uint32 _unique_id;
	uint32 block_to;
	uint32 finish_to;
	task_status _status;
	bool _interruptable;
private:
	static const size_t STACK_SIZE;
	static const size_t PROTECT_SIZE;
	static uint32* free_list_front;
	static uint32* free_list_end;
	static bool initialized;
	friend int __await(event_task*);
	friend int __start(event_task*);
	friend void __cocoflow(event_task*);
	friend void __task_runtime(uint32);
	friend inline bool __task_yield(event_task*);
	friend inline void __task_stand(event_task*);
	friend inline void __task_start_child(event_task*, event_task*);
	friend inline void __task_cancel_children(event_task*, event_task**, uint32);
	template<uint32 UP, uint32 PP> friend inline int await(task<UP, PP>&);
	friend class all_of;
	friend class any_of;
};

class all_of : public event_task
{
public: /* 2~6 argc */
	template<uint32 UP0, uint32 PP0, uint32 UP1, uint32 PP1>
	all_of(task<UP0, PP0>& target0, task<UP1, PP1>& target1);
	template<uint32 UP0, uint32 PP0, uint32 UP1, uint32 PP1, uint32 UP2, uint32 PP2>
	all_of(task<UP0, PP0>& target0, task<UP1, PP1>& target1, task<UP2, PP2>& target2);
	template<uint32 UP0, uint32 PP0, uint32 UP1, uint32 PP1, uint32 UP2, uint32 PP2,
		uint32 UP3, uint32 PP3>
	all_of(task<UP0, PP0>& target0, task<UP1, PP1>& target1, task<UP2, PP2>& target2,
		task<UP3, PP3>& target3);
	template<uint32 UP0, uint32 PP0, uint32 UP1, uint32 PP1, uint32 UP2, uint32 PP2,
		uint32 UP3, uint32 PP3, uint32 UP4, uint32 PP4>
	all_of(task<UP0, PP0>& target0, task<UP1, PP1>& target1, task<UP2, PP2>& target2,
		task<UP3, PP3>& target3, task<UP4, PP4>& target4);
	template<uint32 UP0, uint32 PP0, uint32 UP1, uint32 PP1, uint32 UP2, uint32 PP2,
		uint32 UP3, uint32 PP3, uint32 UP4, uint32 PP4, uint32 UP5, uint32 PP5>
	all_of(task<UP0, PP0>& target0, task<UP1, PP1>& target1, task<UP2, PP2>& target2,
		task<UP3, PP3>& target3, task<UP4, PP4>& target4, task<UP5, PP5>& target5);
	all_of(event_task* targets[], uint32 num);
	virtual ~all_of();
private:
	all_of(const all_of&);
	all_of& operator=(const all_of&);
	virtual void run();
	virtual void cancel();
	const uint32 num;
	event_task** children;
	event_task* _children[6];
};

class any_of : public event_task
{
public: /* 2~6 argc */
	template<uint32 UP0, uint32 PP0, uint32 UP1, uint32 PP1>
	any_of(task<UP0, PP0>& target0, task<UP1, PP1>& target1);
	template<uint32 UP0, uint32 PP0, uint32 UP1, uint32 PP1, uint32 UP2, uint32 PP2>
	any_of(task<UP0, PP0>& target0, task<UP1, PP1>& target1, task<UP2, PP2>& target2);
	template<uint32 UP0, uint32 PP0, uint32 UP1, uint32 PP1, uint32 UP2, uint32 PP2,
		uint32 UP3, uint32 PP3>
	any_of(task<UP0, PP0>& target0, task<UP1, PP1>& target1, task<UP2, PP2>& target2,
		task<UP3, PP3>& target3);
	template<uint32 UP0, uint32 PP0, uint32 UP1, uint32 PP1, uint32 UP2, uint32 PP2,
		uint32 UP3, uint32 PP3, uint32 UP4, uint32 PP4>
	any_of(task<UP0, PP0>& target0, task<UP1, PP1>& target1, task<UP2, PP2>& target2,
		task<UP3, PP3>& target3, task<UP4, PP4>& target4);
	template<uint32 UP0, uint32 PP0, uint32 UP1, uint32 PP1, uint32 UP2, uint32 PP2,
		uint32 UP3, uint32 PP3, uint32 UP4, uint32 PP4, uint32 UP5, uint32 PP5>
	any_of(task<UP0, PP0>& target0, task<UP1, PP1>& target1, task<UP2, PP2>& target2,
		task<UP3, PP3>& target3, task<UP4, PP4>& target4, task<UP5, PP5>& target5);
	any_of(event_task* targets[], uint32 num);
	virtual ~any_of();
	int who_completed();
private:
	any_of(const any_of&);
	any_of& operator=(const any_of&);
	virtual void run();
	virtual void cancel();
	const uint32 num;
	int completed_id;
	event_task** children;
	event_task* _children[6];
};

class sleep : public event_task
{
public:
	sleep(uint64 timeout); //ms
	virtual ~sleep();
private:
	sleep(const sleep&);
	sleep& operator=(const sleep&);
	virtual void run();
	virtual void cancel();
	uint64 timeout;
	void* timer;
};

class sync : public event_task
{
public:
	sync();
	sync(long id); //unique
	virtual ~sync();
	static int notify(sync* obj);
	static int notify(long id);
private:
	sync(const sync&);
	sync& operator=(const sync&);
	virtual void run();
	virtual void cancel();
	long id;
	void* async;
	bool named;
	std::map<long, sync*>::iterator pos;
	static std::map<long, sync*> ids;
};

/*
 * Get sequence from packet
 *     success:(return>=0, *seq to sequence), failure:(return<0)
 * template<typename SeqType>
 * typedef int seq_getter(const void* buf, size_t size, SeqType* seq);
 */

/*
 * template<typename SeqType>
 * typedef void pkg_seq_unrecv(const void* buf, size_t size, const SeqType& seq);
 */

typedef void pkg_seq_failed(const void* buf, size_t size, int ret);

typedef size_t len_getter(const void* buf, size_t size); //failure:(return<size)

typedef void pkg_ignored(const void* buf, size_t size, const struct sockaddr* addr);

class seqer_wrapper_if;

class udp
{
public:
	class send : public event_task
	{
	public:
		send(udp& handle, const struct sockaddr_in& addr, const void* buf, size_t len);
		send(udp& handle, const struct sockaddr_in6& addr, const void* buf, size_t len);
		virtual ~send();
	private:
		send(const send&);
		send& operator=(const send&);
		virtual void run();
		virtual void cancel();
		udp& handle;
		const struct sockaddr_in6 addr;
		uv_buf_t buf;
		friend class udp;
	};
	class recv : public event_task
	{
	public:
		recv(udp& handle, void* buf, size_t& len);
		virtual ~recv();
		uint16 peer_type();
		struct sockaddr_in peer_addr_ipv4();
		struct sockaddr_in6 peer_addr_ipv6();
	private:
		recv(const recv&);
		recv& operator=(const recv&);
		virtual void run();
		virtual void cancel();
		udp& handle;
		void* buf;
		size_t& len;
		struct sockaddr_in6 addr;
		std::list<recv*>::iterator pos;
		friend class udp;
	};
	class recv_by_seq_if : public event_task
	{
	public:
		recv_by_seq_if(udp& handle, void* buf, size_t& len);
		virtual ~recv_by_seq_if();
		uint16 peer_type();
		struct sockaddr_in peer_addr_ipv4();
		struct sockaddr_in6 peer_addr_ipv6();
	protected:
		recv_by_seq_if(const recv_by_seq_if&);
		recv_by_seq_if& operator=(const recv_by_seq_if&);
		void run_part_0();
		void run_part_1();
		udp& handle;
		void* buf;
		size_t& len;
		struct sockaddr_in6 addr;
		friend class udp;
	};
	template<typename SeqType = uint32>
	class recv_by_seq : public recv_by_seq_if
	{
	public:
		inline recv_by_seq(udp& handle, void* buf, size_t& len, const SeqType& seq);
		virtual ~recv_by_seq() {}
	private:
		recv_by_seq(const recv_by_seq&);
		recv_by_seq& operator=(const recv_by_seq&);
		virtual void run();
		virtual void cancel();
		const SeqType seq;
		static std::set<void*> type_check;
	};
	typedef recv_by_seq<> recv_by_seq_u32;
	udp();
	~udp();
	int bind(const struct sockaddr_in& addr);
	int bind(const struct sockaddr_in6& addr, bool ipv6_only = false);
	template<typename SeqType>
	int bind(
		int (*getter)(const void*, size_t, SeqType*), //seq_getter* getter
		void (*unrecv)(const void*, size_t, const SeqType&) = NULL, //pkg_seq_unrecv* unrecv
		pkg_seq_failed* failed = NULL
	);
	template<typename Compare, typename SeqType>
	int bind(
		int (*getter)(const void*, size_t, SeqType*), //seq_getter* getter
		void (*unrecv)(const void*, size_t, const SeqType&) = NULL, //pkg_seq_unrecv* unrecv
		pkg_seq_failed* failed = NULL
	);
	void ignore_recv(pkg_ignored* ignored = NULL);
	unsigned long long count_unrecv() const;
	unsigned long long count_failed() const;
	unsigned long long count_ignored() const;
	static const void* internal_buffer(size_t& len);
private:
	udp(const udp&);
	udp& operator=(const udp&);
	void* sock;
	unsigned char receiving;
	recv* cur_alloc;
	seqer_wrapper_if* seqer;
	pkg_ignored* ignored;
	unsigned long long c_unrecv;
	unsigned long long c_failed;
	unsigned long long c_ignored;
	std::list<recv*> recv_queue;
	static char routing_buf[65536];
	static size_t routing_len;
	static uv_buf_t udp_alloc_cb0(uv_handle_t*, size_t);
	static void udp_recv_cb0(uv_udp_t*, ssize_t, uv_buf_t, struct sockaddr*, unsigned);
	static uv_buf_t udp_alloc_cb1(uv_handle_t*, size_t);
	static void udp_recv_cb1(uv_udp_t*, ssize_t, uv_buf_t, struct sockaddr*, unsigned);
	static uv_buf_t udp_alloc_cb2(uv_handle_t*, size_t);
	static void udp_recv_cb2(uv_udp_t*, ssize_t, uv_buf_t, struct sockaddr*, unsigned);
};

namespace tcp {

enum {
	success = 0,
	/* -1 ~ -20 basic error */
	unfinished = -1,
	failure = -2, //unknown
	/* -21 ~ -30 accept error */
	address_in_use = -21, //address already in use
	/* -31 ~ -40 connect error */
	/* -41 ~ -60 send error */
	/* -61 ~ -80 recv error */
	packet_length_too_short = -61, //len_getter return length < min_len
	packet_length_too_long = -62 //len_getter return length > max_len
};

class accept;

class listening
{
public:
	listening(int backlog = 64);
	~listening();
	int bind(const struct sockaddr_in& addr);
	int bind(const struct sockaddr_in6& addr);
private:
	listening(const listening&);
	listening& operator=(const listening&);
	void* sock;
	int backlog;
	bool accepting;
	std::list<accept*> accept_queue;
	static void tcp_accept_cb(uv_stream_t*, int);
	friend class accept;
};

class recv;
class recv_till;

class connected
{
public:
	connected();
	~connected();
	template<typename SeqType>
	int bind(
		size_t min_len,
		size_t max_len,
		len_getter* lener,
		int (*getter)(const void*, size_t, SeqType*), //seq_getter* getter
		void (*unrecv)(const void*, size_t, const SeqType&) = NULL, //pkg_seq_unrecv* unrecv
		pkg_seq_failed* failed = NULL
	);
	template<typename Compare, typename SeqType>
	int bind(
		size_t min_len,
		size_t max_len,
		len_getter* lener,
		int (*getter)(const void*, size_t, SeqType*), //seq_getter* getter
		void (*unrecv)(const void*, size_t, const SeqType&) = NULL, //pkg_seq_unrecv* unrecv
		pkg_seq_failed* failed = NULL
	);
	uint16 peer_type();
	struct sockaddr_in peer_addr_ipv4();
	struct sockaddr_in6 peer_addr_ipv6();
	unsigned long long count_unrecv() const;
	unsigned long long count_failed() const;
	const void* internal_buffer(size_t& len); //Just for recv_by_seq
private:
	connected(const connected&);
	connected& operator=(const connected&);
	int bind_inner(size_t, size_t, len_getter*);
	void* sock;
	bool established;
	bool broken;
	unsigned char receiving;
	std::list<recv*> recv_queue0;
	std::list<recv_till*> recv_queue1;
	recv* cur_alloc0;
	recv_till* cur_alloc1;
	char* buf1;
	size_t size1;
	size_t len1;
	char* buf2;
	size_t size2;
	size_t len2;
	size_t header_len;
	size_t packet_len;
	len_getter* lener;
	seqer_wrapper_if* seqer;
	unsigned long long c_unrecv;
	unsigned long long c_failed;
	void* async_cancel1;
	static void tcp_connect_cb(uv_connect_t*, int);
	static void tcp_send_cb(uv_write_t*, int);
	static uv_buf_t tcp_alloc_cb0(uv_handle_t*, size_t);
	static void tcp_recv_cb0(uv_stream_t*, ssize_t, uv_buf_t);
	static uv_buf_t tcp_alloc_cb1(uv_handle_t*, size_t);
	static void tcp_recv_cb1(uv_stream_t*, ssize_t, uv_buf_t);
	static uv_buf_t tcp_alloc_cb2(uv_handle_t*, size_t);
	static void tcp_recv_cb2(uv_stream_t*, ssize_t, uv_buf_t);
	static void tcp_fallback_cb1(uv_async_t*, int);
	static void check_remaining(uv_handle_t*);
	static void break_all_recv(uv_handle_t*, int);
	static bool break_all_recv_by_seq(void*, void*);
	friend class listening;
	friend class connect;
	friend class send;
	friend class recv;
	friend class recv_till;
	friend class recv_by_seq_if;
	template<typename SeqType> friend class recv_by_seq;
};

class accept : public event_task
{
public:
	accept(int& ret, listening& handle, connected& conn);
	virtual ~accept();
private:
	accept(const accept&);
	accept& operator=(const accept&);
	virtual void run();
	virtual void cancel();
	int& ret;
	listening& handle;
	connected& conn;
	std::list<accept*>::iterator pos;
	friend class listening;
};

class connect : public event_task
{
public:
	connect(int& ret, connected& handle, const struct sockaddr_in& addr);
	connect(int& ret, connected& handle, const struct sockaddr_in6& addr);
	virtual ~connect();
private:
	connect(const connect&);
	connect& operator=(const connect&);
	virtual void run();
	virtual void cancel();
	void *req;
	int& ret;
	connected& handle;
	const struct sockaddr_in6 addr;
	friend class connected;
};

class send : public event_task
{
public: /* 1~4 bufs */
	send(int& ret, connected& handle, const void* buf0, size_t len0);
	send(int& ret, connected& handle, const void* buf0, size_t len0, const void* buf1, size_t len1);
	send(int& ret, connected& handle, const void* buf0, size_t len0, const void* buf1, size_t len1, const void* buf2, size_t len2);
	send(int& ret, connected& handle, const void* buf0, size_t len0, const void* buf1, size_t len1, const void* buf2, size_t len2, const void* buf3, size_t len3);
	virtual ~send();
private:
	send(const send&);
	send& operator=(const send&);
	virtual void run();
	virtual void cancel();
	int& ret;
	connected& handle;
	const uint32 num;
	uv_buf_t buf[4];
	friend class connected;
};

class recv : public event_task
{
public:
	recv(int& ret, connected& handle, void* buf, size_t& len);
	virtual ~recv();
private:
	recv(const recv&);
	recv& operator=(const recv&);
	virtual void run();
	virtual void cancel();
	int& ret;
	connected& handle;
	void* buf;
	size_t& len;
	std::list<recv*>::iterator pos;
	friend class connected;
};

class recv_till : public event_task
{
public:
	recv_till(int& ret, connected& handle, void* buf, size_t& len); //till fill up buf
	recv_till(int& ret, connected& handle, void* buf, size_t& len, const void* pattern, size_t pattern_len); //till end with pattern
	virtual ~recv_till();
private:
	recv_till(const recv_till&);
	recv_till& operator=(const recv_till&);
	virtual void run();
	virtual void cancel();
	bool remaining();
	int& ret;
	connected& handle;
	void* const buf;
	size_t& len;
	const void* pattern;
	size_t pattern_len;
	void* cur;
	size_t left;
	std::list<recv_till*>::iterator pos;
	friend class connected;
};

class recv_by_seq_if : public event_task
{
public:
	recv_by_seq_if(int& ret, connected& handle, void* buf, size_t& len);
	virtual ~recv_by_seq_if();
protected:
	recv_by_seq_if(const recv_by_seq_if&);
	recv_by_seq_if& operator=(const recv_by_seq_if&);
	void run_part_0();
	void run_part_1();
	int& ret;
	connected& handle;
	void* buf;
	size_t& len;
	friend class connected;
};

template<typename SeqType = uint32>
class recv_by_seq : public recv_by_seq_if
{
public:
	inline recv_by_seq(int& ret, connected& handle, void* buf, size_t& len, const SeqType& seq);
	virtual ~recv_by_seq() {}
private:
	recv_by_seq(const recv_by_seq&);
	recv_by_seq& operator=(const recv_by_seq&);
	virtual void run();
	virtual void cancel();
	const SeqType seq;
	static std::set<void*> type_check;
};

typedef recv_by_seq<> recv_by_seq_u32;

} /* end of namespace tcp */

struct sockaddr_in ip_to_addr(const char* ipv4, int port);
struct sockaddr_in6 ip_to_addr6(const char* ipv6, int port);
std::string ip_to_str(const struct sockaddr* addr);
std::string ip_to_str(const struct sockaddr_in& addr);
std::string ip_to_str(const struct sockaddr_in6& addr);

void set_debug(FILE* fp);

/********** Internal implementation (User no care) **********/

#if defined(__GNUC__)
# define ccf_likely(x)   (__builtin_expect(!!(x),1))
# define ccf_unlikely(x) (__builtin_expect(!!(x),0))
#else
# define ccf_likely(x)   (x)
# define ccf_unlikely(x) (x)
#endif

#if !defined(_MSC_VER)
# define CCF_FATAL_ERROR(fmt, args...) \
do { \
	fprintf(stderr, "[FATAL]: " fmt "\n", ##args); \
	abort(); \
} while(0)
#else
# define CCF_FATAL_ERROR(fmt, ...) \
do { \
	fprintf(stderr, "[FATAL]: " fmt "\n", __VA_ARGS__); \
	abort(); \
} while(0)
#endif

class seqer_wrapper_if
{
protected:
	seqer_wrapper_if() {}
	virtual ~seqer_wrapper_if() {}
	virtual void insert(const void*, void*) = 0;
	virtual void erase(const void*) = 0;
	virtual int unwrap(const void*, size_t, void**) = 0;
	virtual void call_unrecv(const void*, size_t) const = 0;
	virtual void call_failed(const void*, size_t, int) const = 0;
	virtual void drop_all(bool (*)(void*, void*), void*) = 0;
	virtual bool check_type_for_udp(void*) = 0;
	virtual bool check_type_for_tcp(void*) = 0;
	friend class udp;
	friend class tcp::connected;
	template<typename SeqType> friend class tcp::recv_by_seq;
};

static const uint32 EVENT_LOOP_ID = 0xffffffff;

extern bool global_initialized;
extern event_task** global_task_manager;

template<uint32 UserPages, uint32 ProtectPages>
const size_t _task_tpl::STACK_SIZE = (UserPages + ProtectPages) * PAGE_SIZE;

template<uint32 UserPages, uint32 ProtectPages>
const size_t _task_tpl::PROTECT_SIZE = ProtectPages * PAGE_SIZE;

template<uint32 UserPages, uint32 ProtectPages>
uint32* _task_tpl::free_list_front = NULL;

template<uint32 UserPages, uint32 ProtectPages>
uint32* _task_tpl::free_list_end = NULL;

template<uint32 UserPages, uint32 ProtectPages>
bool _task_tpl::initialized = false;

int __await(event_task*);

template<uint32 UserPages, uint32 ProtectPages>
int await(_task_tpl& target)
{
	if (ccf_likely(target._unique_id != EVENT_LOOP_ID))
	{
		*(--_task_tpl::free_list_front) = target._unique_id;
		target._unique_id = EVENT_LOOP_ID;
	}
	return __await(reinterpret_cast<event_task*>(&target));
}

int __start(event_task*);

template<uint32 UserPages, uint32 ProtectPages>
int start(_task_tpl* target)
{
	return __start(reinterpret_cast<event_task*>(target));
}

void __cocoflow(event_task*);

template<uint32 UserPages, uint32 ProtectPages>
void cocoflow(_task_tpl& top)
{
	__cocoflow(reinterpret_cast<event_task*>(&top));
}

void __init();

template<uint32 UserPages, uint32 ProtectPages>
_task_tpl::task() : reuse(NULL), _interruptable(true)
{
	if (ccf_unlikely(!global_initialized))
		__init();
	
	if (ccf_likely(_task_tpl::initialized))
	{
		if (ccf_likely(_task_tpl::free_list_front != _task_tpl::free_list_end))
		{
			this->_unique_id = *(_task_tpl::free_list_front++);
			this->_status = ready;
			global_task_manager[this->_unique_id] = reinterpret_cast<event_task*>(this);
		}
		else
		{
			this->_unique_id = EVENT_LOOP_ID;
			this->_status = limited;
		}
	}
	else
		CCF_FATAL_ERROR("Create a task before initializing in task<%u, %u>", UserPages, ProtectPages);
}

template<uint32 UserPages, uint32 ProtectPages>
void _task_tpl::cancel()
{
}

template<uint32 UserPages, uint32 ProtectPages>
_task_tpl::~task()
{
	if (this->_unique_id != EVENT_LOOP_ID)
		*(--_task_tpl::free_list_front) = this->_unique_id;
}

template<uint32 UserPages, uint32 ProtectPages>
task_status _task_tpl::status() const
{
	return this->_status;
}

template<uint32 UserPages, uint32 ProtectPages>
uint32 _task_tpl::unique_id() const
{
	return this->_unique_id;
}

template<uint32 UserPages, uint32 ProtectPages>
inline void _task_tpl::uninterruptable()
{
	this->_interruptable = false;
}

void __init_setting(uint32* &, uint32* &, uint32, uint32, uint32);

template<uint32 UserPages, uint32 ProtectPages>
void _task_tpl::init(uint32 max_task_num)
{
	if (initialized)
		CCF_FATAL_ERROR("Initializing task<%u, %u> after creating a task", UserPages, ProtectPages);
	
	if (_task_tpl::initialized)
		CCF_FATAL_ERROR("Duplicate initializing in task<%u, %u>", UserPages, ProtectPages);
	
	if (!max_task_num)
		CCF_FATAL_ERROR("Illegal number in initializing task<%u, %u>", UserPages, ProtectPages);
	
	__init_setting(_task_tpl::free_list_front, _task_tpl::free_list_end, _task_tpl::STACK_SIZE, _task_tpl::PROTECT_SIZE, max_task_num);
	
	_task_tpl::initialized = true;
}

template<uint32 UP0, uint32 PP0, uint32 UP1, uint32 PP1>
all_of::all_of(task<UP0, PP0>& target0, task<UP1, PP1>& target1) : num(2)
{
	this->children = this->_children;
	this->children[0] = reinterpret_cast<event_task*>(&target0);
	this->children[1] = reinterpret_cast<event_task*>(&target1);
}

template<uint32 UP0, uint32 PP0, uint32 UP1, uint32 PP1, uint32 UP2, uint32 PP2>
all_of::all_of(task<UP0, PP0>& target0, task<UP1, PP1>& target1, task<UP2, PP2>& target2) : num(3)
{
	this->children = this->_children;
	this->children[0] = reinterpret_cast<event_task*>(&target0);
	this->children[1] = reinterpret_cast<event_task*>(&target1);
	this->children[2] = reinterpret_cast<event_task*>(&target2);
}

template<uint32 UP0, uint32 PP0, uint32 UP1, uint32 PP1, uint32 UP2, uint32 PP2,
	uint32 UP3, uint32 PP3>
all_of::all_of(task<UP0, PP0>& target0, task<UP1, PP1>& target1, task<UP2, PP2>& target2,
	task<UP3, PP3>& target3) : num(4)
{
	this->children = this->_children;
	this->children[0] = reinterpret_cast<event_task*>(&target0);
	this->children[1] = reinterpret_cast<event_task*>(&target1);
	this->children[2] = reinterpret_cast<event_task*>(&target2);
	this->children[3] = reinterpret_cast<event_task*>(&target3);
}

template<uint32 UP0, uint32 PP0, uint32 UP1, uint32 PP1, uint32 UP2, uint32 PP2,
	uint32 UP3, uint32 PP3, uint32 UP4, uint32 PP4>
all_of::all_of(task<UP0, PP0>& target0, task<UP1, PP1>& target1, task<UP2, PP2>& target2,
	task<UP3, PP3>& target3, task<UP4, PP4>& target4) : num(5)
{
	this->children = this->_children;
	this->children[0] = reinterpret_cast<event_task*>(&target0);
	this->children[1] = reinterpret_cast<event_task*>(&target1);
	this->children[2] = reinterpret_cast<event_task*>(&target2);
	this->children[3] = reinterpret_cast<event_task*>(&target3);
	this->children[4] = reinterpret_cast<event_task*>(&target4);
}

template<uint32 UP0, uint32 PP0, uint32 UP1, uint32 PP1, uint32 UP2, uint32 PP2,
	uint32 UP3, uint32 PP3, uint32 UP4, uint32 PP4, uint32 UP5, uint32 PP5>
all_of::all_of(task<UP0, PP0>& target0, task<UP1, PP1>& target1, task<UP2, PP2>& target2,
	task<UP3, PP3>& target3, task<UP4, PP4>& target4, task<UP5, PP5>& target5) : num(6)
{
	this->children = this->_children;
	this->children[0] = reinterpret_cast<event_task*>(&target0);
	this->children[1] = reinterpret_cast<event_task*>(&target1);
	this->children[2] = reinterpret_cast<event_task*>(&target2);
	this->children[3] = reinterpret_cast<event_task*>(&target3);
	this->children[4] = reinterpret_cast<event_task*>(&target4);
	this->children[5] = reinterpret_cast<event_task*>(&target5);
}

template<uint32 UP0, uint32 PP0, uint32 UP1, uint32 PP1>
any_of::any_of(task<UP0, PP0>& target0, task<UP1, PP1>& target1) : num(2), completed_id(-1)
{
	this->children = this->_children;
	this->children[0] = reinterpret_cast<event_task*>(&target0);
	this->children[1] = reinterpret_cast<event_task*>(&target1);
}

template<uint32 UP0, uint32 PP0, uint32 UP1, uint32 PP1, uint32 UP2, uint32 PP2>
any_of::any_of(task<UP0, PP0>& target0, task<UP1, PP1>& target1, task<UP2, PP2>& target2) : num(3), completed_id(-1)
{
	this->children = this->_children;
	this->children[0] = reinterpret_cast<event_task*>(&target0);
	this->children[1] = reinterpret_cast<event_task*>(&target1);
	this->children[2] = reinterpret_cast<event_task*>(&target2);
}

template<uint32 UP0, uint32 PP0, uint32 UP1, uint32 PP1, uint32 UP2, uint32 PP2,
	uint32 UP3, uint32 PP3>
any_of::any_of(task<UP0, PP0>& target0, task<UP1, PP1>& target1, task<UP2, PP2>& target2,
	task<UP3, PP3>& target3) : num(4), completed_id(-1)
{
	this->children = this->_children;
	this->children[0] = reinterpret_cast<event_task*>(&target0);
	this->children[1] = reinterpret_cast<event_task*>(&target1);
	this->children[2] = reinterpret_cast<event_task*>(&target2);
	this->children[3] = reinterpret_cast<event_task*>(&target3);
}

template<uint32 UP0, uint32 PP0, uint32 UP1, uint32 PP1, uint32 UP2, uint32 PP2,
	uint32 UP3, uint32 PP3, uint32 UP4, uint32 PP4>
any_of::any_of(task<UP0, PP0>& target0, task<UP1, PP1>& target1, task<UP2, PP2>& target2,
	task<UP3, PP3>& target3, task<UP4, PP4>& target4) : num(5), completed_id(-1)
{
	this->children = this->_children;
	this->children[0] = reinterpret_cast<event_task*>(&target0);
	this->children[1] = reinterpret_cast<event_task*>(&target1);
	this->children[2] = reinterpret_cast<event_task*>(&target2);
	this->children[3] = reinterpret_cast<event_task*>(&target3);
	this->children[4] = reinterpret_cast<event_task*>(&target4);
}

template<uint32 UP0, uint32 PP0, uint32 UP1, uint32 PP1, uint32 UP2, uint32 PP2,
	uint32 UP3, uint32 PP3, uint32 UP4, uint32 PP4, uint32 UP5, uint32 PP5>
any_of::any_of(task<UP0, PP0>& target0, task<UP1, PP1>& target1, task<UP2, PP2>& target2,
	task<UP3, PP3>& target3, task<UP4, PP4>& target4, task<UP5, PP5>& target5) : num(6), completed_id(-1)
{
	this->children = this->_children;
	this->children[0] = reinterpret_cast<event_task*>(&target0);
	this->children[1] = reinterpret_cast<event_task*>(&target1);
	this->children[2] = reinterpret_cast<event_task*>(&target2);
	this->children[3] = reinterpret_cast<event_task*>(&target3);
	this->children[4] = reinterpret_cast<event_task*>(&target4);
	this->children[5] = reinterpret_cast<event_task*>(&target5);
}

template<typename SeqType, typename Compare>
class seqer_wrapper : public seqer_wrapper_if
{
protected:
	seqer_wrapper(
		int  (*getter)(const void*, size_t, SeqType*),
		void (*unrecv)(const void*, size_t, const SeqType&),
		void (*failed)(const void*, size_t, int)
	) : cur_seq(), getter(getter), unrecv(unrecv), failed(failed) {}
	virtual ~seqer_wrapper() {}
	virtual void insert(const void* seq, void* obj)
	{
		(void)this->seq_mapping.insert(std::pair<SeqType, void*>(*reinterpret_cast<const SeqType*>(seq), obj));
	}
	virtual void erase(const void* seq)
	{
		(void)this->seq_mapping.erase(*reinterpret_cast<const SeqType*>(seq));
	}
	virtual int unwrap(const void* buf, size_t size, void** obj)
	{
		int ret = this->getter(buf, size, &this->cur_seq);
		if (ret >= 0)
		{
			typename std::multimap<SeqType, void*, Compare>::iterator it = this->seq_mapping.find(this->cur_seq);
			if (it != this->seq_mapping.end())
			{
				*obj = it->second;
				(void)this->seq_mapping.erase(it);
				return ret;
			}
		}
		*obj = NULL;
		return ret;
	}
	virtual void call_unrecv(const void* buf, size_t size) const
	{
		if (this->unrecv)
			this->unrecv(buf, size, this->cur_seq);
	}
	virtual void call_failed(const void* buf, size_t size, int ret) const
	{
		if (this->failed)
			this->failed(buf, size, ret);
	}
	virtual void drop_all(bool (*cb)(void*, void*), void* data)
	{
		for (typename std::multimap<SeqType, void*, Compare>::iterator it = this->seq_mapping.begin(); it != this->seq_mapping.end(); )
		{
			void* obj = it->second;
			this->seq_mapping.erase(it++);
			if (cb(obj, data))
				break;
		}
	}
	virtual bool check_type_for_udp(void* rbs)
	{
		return dynamic_cast<udp::recv_by_seq<SeqType>*>(reinterpret_cast<udp::recv_by_seq_if*>(rbs)) != NULL;
	}
	virtual bool check_type_for_tcp(void* rbs)
	{
		return dynamic_cast<tcp::recv_by_seq<SeqType>*>(reinterpret_cast<tcp::recv_by_seq_if*>(rbs)) != NULL;
	}
private:
	std::multimap<SeqType, void*, Compare> seq_mapping;
	SeqType cur_seq;
	int  (*getter)(const void*, size_t, SeqType*);
	void (*unrecv)(const void*, size_t, const SeqType&);
	void (*failed)(const void*, size_t, int);
	friend class udp;
	friend class tcp::connected;
};

template<typename SeqType>
int udp::bind(
	int  (*getter)(const void*, size_t, SeqType*),
	void (*unrecv)(const void*, size_t, const SeqType&),
	void (*failed)(const void*, size_t, int)
)
{
	if (!getter || this->seqer)
		return -1;
	this->seqer = new seqer_wrapper< SeqType, std::less<SeqType> >(getter, unrecv, failed);
	return 0;
}

template<typename Compare, typename SeqType>
int udp::bind(
	int  (*getter)(const void*, size_t, SeqType*),
	void (*unrecv)(const void*, size_t, const SeqType&),
	void (*failed)(const void*, size_t, int)
)
{
	if (!getter || this->seqer)
		return -1;
	this->seqer = new seqer_wrapper<SeqType, Compare>(getter, unrecv, failed);
	return 0;
}

template<typename SeqType>
std::set<void*> udp::recv_by_seq<SeqType>::type_check;

template<typename SeqType>
udp::recv_by_seq<SeqType>::recv_by_seq(udp& handle, void* buf, size_t& len, const SeqType& seq)
	: recv_by_seq_if(handle, buf, len), seq(seq)
{
#if !defined(disable_check_seq_type)
	if (ccf_unlikely(!udp::recv_by_seq<SeqType>::type_check.count(&handle)))
	{
		if (this->handle.seqer->check_type_for_udp(this))
			(void)udp::recv_by_seq<SeqType>::type_check.insert(&handle);
		else
			CCF_FATAL_ERROR("Check seq type failed in udp::recv_by_seq<SeqType>");
	}
#endif
}

template<typename SeqType>
void udp::recv_by_seq<SeqType>::run()
{
	this->run_part_0();
	this->handle.seqer->insert(&this->seq, this);
	this->run_part_1();
}

template<typename SeqType>
void udp::recv_by_seq<SeqType>::cancel()
{
	this->handle.seqer->erase(&this->seq);
}

namespace tcp {

template<typename SeqType>
int connected::bind(
	size_t min_len,
	size_t max_len,
	len_getter* lener,
	int  (*getter)(const void*, size_t, SeqType*),
	void (*unrecv)(const void*, size_t, const SeqType&),
	void (*failed)(const void*, size_t, int)
)
{
	if (!getter || this->seqer)
		return -1;
	this->seqer = new seqer_wrapper< SeqType, std::less<SeqType> >(getter, unrecv, failed);
	return this->bind_inner(min_len, max_len, lener);
}

template<typename Compare, typename SeqType>
int connected::bind(
	size_t min_len,
	size_t max_len,
	len_getter* lener,
	int  (*getter)(const void*, size_t, SeqType*),
	void (*unrecv)(const void*, size_t, const SeqType&),
	void (*failed)(const void*, size_t, int)
)
{
	if (!getter || this->seqer)
		return -1;
	this->seqer = new seqer_wrapper<SeqType, Compare>(getter, unrecv, failed);
	return this->bind_inner(min_len, max_len, lener);
}

template<typename SeqType>
std::set<void*> recv_by_seq<SeqType>::type_check;

template<typename SeqType>
recv_by_seq<SeqType>::recv_by_seq(int& ret, connected& handle, void* buf, size_t& len, const SeqType& seq)
	: recv_by_seq_if(ret, handle, buf, len), seq(seq)
{
#if !defined(disable_check_seq_type)
	if (ccf_unlikely(!recv_by_seq<SeqType>::type_check.count(&handle)))
	{
		if (this->handle.seqer->check_type_for_tcp(this))
			(void)recv_by_seq<SeqType>::type_check.insert(&handle);
		else
			CCF_FATAL_ERROR("Check seq type failed in tcp::recv_by_seq<SeqType>");
	}
#endif
}

template<typename SeqType>
void recv_by_seq<SeqType>::run()
{
	this->run_part_0();
	this->handle.seqer->insert(&this->seq, this);
	this->run_part_1();
}

template<typename SeqType>
void recv_by_seq<SeqType>::cancel()
{
	this->handle.seqer->erase(&this->seq);
}

}

#undef _task_tpl
#undef CCF_FATAL_ERROR

} /* end of namespace ccf */

#endif
