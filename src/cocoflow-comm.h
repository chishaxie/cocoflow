#ifndef __COCOFLOW_COMM_H__
#define __COCOFLOW_COMM_H__

#include <time.h>

#if !defined(_WIN32) && !defined(_WIN64)
# include <ucontext.h>
# include <fcntl.h>
# include <sys/mman.h>
#endif

extern "C" {
#define __disable_simplify_uv_h__
#include "uv/uv.h"
}

#include "cocoflow.h"

#if !defined(_MSC_VER)
	#define FATAL_ERROR(fmt, args...) \
	do { \
		fprintf(stderr, "[FATAL]: " fmt "\n", ##args); \
		abort(); \
	} while(0)
	#define LOG_DEBUG(fmt, args...) \
	do { \
		uint32 ns = uv_hrtime()%1000000000; \
		time_t s = time(NULL); \
		struct tm date = *localtime(&s); \
		fprintf(global_debug_file, "[%02u:%02u:%02u.%u] [DEBUG]: " fmt "\n", date.tm_hour, date.tm_min, date.tm_sec, ns, ##args); \
	} while(0)
#else
	#pragma warning(disable:4996)
	#define FATAL_ERROR(fmt, ...) \
	do { \
		fprintf(stderr, "[FATAL]: " fmt "\n", __VA_ARGS__); \
		abort(); \
	} while(0)
	#define LOG_DEBUG(fmt, ...) \
	do { \
		uint32 ns = uv_hrtime()%1000000000; \
		time_t s = time(NULL); \
		struct tm date = *localtime(&s); \
		fprintf(global_debug_file, "[%02u:%02u:%02u.%u] [DEBUG]: " fmt "\n", date.tm_hour, date.tm_min, date.tm_sec, ns, __VA_ARGS__); \
	} while(0)
#endif

#define CHECK(x) \
do { \
	if (ccf_unlikely(!(x))) \
	{ \
		fprintf(stderr, "[ASSERT]: " #x " failed at " __FILE__ ":%u\n", __LINE__); \
		abort(); \
	} \
} while(0)

namespace ccf {

#if defined(_WIN32) || defined(_WIN64)
	typedef LPVOID coroutine;
	#define coroutine_by_thread(runtime) \
	do { \
		(*(runtime)) = ConvertThreadToFiberEx(NULL, FIBER_FLAG_FLOAT_SWITCH); \
		CHECK((*(runtime)) != NULL); \
	} while(0)
	#define coroutine_create(runtime, mem, size, id) \
	do { \
		(*(runtime)) = CreateFiberEx(size, size, FIBER_FLAG_FLOAT_SWITCH, reinterpret_cast<LPFIBER_START_ROUTINE>(__task_runtime), reinterpret_cast<LPVOID>(id)); \
		CHECK((*(runtime)) != NULL); \
	} while(0)
	#define coroutine_switch(from, to, from_id, to_id) \
	do { \
		(*from) = GetCurrentFiber(); \
		CHECK((*from) != NULL); \
		SwitchToFiber(*to); \
	} while(0)
	#define coroutine_memory_alloc(total_size) reinterpret_cast<void*>(0x1000)
	#define coroutine_memory_protect(mem, size) ;
#else
	typedef ucontext_t coroutine;
	#define coroutine_by_thread(runtime) ;
	#define coroutine_create(runtime, mem, size, id) \
	do { \
		CHECK(getcontext(runtime) == 0); \
		(runtime)->uc_link = &global_loop_running; \
		(runtime)->uc_stack.ss_sp = (mem); \
		(runtime)->uc_stack.ss_size = (size); \
		makecontext(runtime, reinterpret_cast<void (*)()>(__task_runtime), 1, (long)id); \
	} while(0)
	#define coroutine_switch(from, to, from_id, to_id) \
	do { \
		if (ccf_unlikely(swapcontext(from, to))) \
			FATAL_ERROR("Swap running failed, %u -> %u", from_id, to_id); \
	} while(0)
	#define coroutine_memory_alloc(total_size) \
		mmap(NULL, total_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)
	#define coroutine_memory_protect(mem, size) \
	do { \
		CHECK(mprotect(mem, size, PROT_NONE) == 0); \
	} while(0)
#endif

extern coroutine*   global_running_manager;
extern coroutine    global_loop_running;
extern event_task*  global_current_task;
extern bool         global_signal_canceled;
extern FILE*        global_debug_file;

static inline uv_loop_t* loop()
{
	static uv_loop_t* __loop = NULL;
	if (ccf_unlikely(__loop == NULL))
		__loop = uv_default_loop();
	return __loop;
}

struct interrupt_canceled
{
	interrupt_canceled(int level) : level(level) {}
	const int level;
};

static inline void swap_running(uint32 cur, uint32 next)
{
	coroutine *cur_runing = (cur == EVENT_LOOP_ID) ? (&global_loop_running) : (global_running_manager + cur),
			  *next_runing = (next == EVENT_LOOP_ID) ? (&global_loop_running) : (global_running_manager + next);
	global_current_task = (next == EVENT_LOOP_ID) ? (NULL) : (global_task_manager[next]);
	if (ccf_unlikely(global_debug_file))
	{
		if (cur == EVENT_LOOP_ID)
			LOG_DEBUG("Task switch event_loop -> %u", next);
		else if (next == EVENT_LOOP_ID)
			LOG_DEBUG("Task switch %u -> event_loop", cur);
		else
			LOG_DEBUG("Task switch %u -> %u", cur, next);
	}
	coroutine_switch(cur_runing, next_runing, cur, next);
	global_current_task = (cur == EVENT_LOOP_ID) ? (NULL) : (global_task_manager[cur]);
}

/* false means interrupt by cancel */
inline bool __task_yield(event_task* cur)
{
	uint32 source = cur->_unique_id, target = cur->block_to;
	cur->block_to = EVENT_LOOP_ID; //switch to block_to only once
	swap_running(source, target);
	if (global_signal_canceled)
	{
		global_signal_canceled = false;
		if (cur->_status != canceled) //Indirect
		{
			cur->_status = canceled;
			throw interrupt_canceled(0);
		}
		else
			cur->cancel();
		return false;
	}
	else
		return true;
}

inline void __task_stand(event_task* cur)
{
	swap_running(EVENT_LOOP_ID, cur->_unique_id);
}

/***** uv *****/

void free_self_close_cb(uv_handle_t* handle);

/***** sockaddr *****/

struct sockaddr_in6 sockaddr_in_into_sockaddr_in6(const struct sockaddr_in& addr);
struct sockaddr_in sockaddr_in_outof_sockaddr_in6(const struct sockaddr_in6& addr);

}

#endif
