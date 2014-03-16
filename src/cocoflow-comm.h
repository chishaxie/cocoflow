#ifndef __COCOFLOW_COMM_H__
#define __COCOFLOW_COMM_H__

#include <typeinfo>
#include <time.h>

#if !defined(_WIN32) && !defined(_WIN64)
# include <ucontext.h>
# include <fcntl.h>
# include <sys/mman.h>
#endif

#if defined(__GNUG__)
# include <cxxabi.h>
#endif

extern "C" {
#define __disable_simplify_uv_h__
#include "uv/uv.h"
}

#include "cocoflow.h"

#define CLASS_TIPS_MAX_LEN 2048

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
		fprintf(global_debug_file, "[%02u:%02u:%02u.%09u] [DEBUG]: " fmt "\n", date.tm_hour, date.tm_min, date.tm_sec, ns, ##args); \
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
		fprintf(global_debug_file, "[%02u:%02u:%02u.%09u] [DEBUG]: " fmt "\n", date.tm_hour, date.tm_min, date.tm_sec, ns, __VA_ARGS__); \
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

#define task_set_status(task, status) \
do { \
	(task)->_info = ((task)->_info & 0xf0) | (status); \
} while(0)
#define task_get_status(task) ((task)->_info & 0x0f)

#define task_info_uninterruptable  0x10
#define task_info_all_any          0x20

#define task_set_uninterruptable(task) \
do { \
	(task)->_info |= task_info_uninterruptable; \
} while(0)
#define task_is_uninterruptable(task) ((task)->_info & task_info_uninterruptable)

#define task_set_all_any(task) \
do { \
	(task)->_info |= task_info_all_any; \
} while(0)
#define task_is_all_any(task) ((task)->_info & task_info_all_any)

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
extern char         global_debug_output_src[CLASS_TIPS_MAX_LEN];
extern char         global_debug_output_dst[CLASS_TIPS_MAX_LEN];

static inline const char* et_to_tips(const event_task* et, char* tips)
{
	if (et != NULL)
	{
#if defined(__GNUG__)
		size_t len = CLASS_TIPS_MAX_LEN;
		abi::__cxa_demangle(typeid(*et).name(), tips, &len, NULL);
#else
		strcpy(tips, typeid(*et).name());
#endif
	}
	else
		strcpy(tips, "???");
	return tips;
}

#define src_to_tips(src) et_to_tips(src, global_debug_output_src)
#define dst_to_tips(dst) et_to_tips(dst, global_debug_output_dst)

static inline uv_loop_t* loop()
{
	static uv_loop_t* __loop = NULL;
	if (ccf_unlikely(__loop == NULL))
		__loop = uv_default_loop();
	return __loop;
}

#define interrupt_canceled _ic

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
			LOG_DEBUG("[Switch]          EventLoop -> %u-<%s>", next, dst_to_tips(global_task_manager[next]));
		else if (next == EVENT_LOOP_ID)
			LOG_DEBUG("[Switch]          %u-<%s> -> EventLoop", cur, src_to_tips(global_task_manager[cur]));
		else
			LOG_DEBUG("[Switch]          %u-<%s> -> %u-<%s>", cur, src_to_tips(global_task_manager[cur]), next, dst_to_tips(global_task_manager[next]));
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
		if (task_get_status(cur) != canceled) //Indirect
		{
			task_set_status(cur, canceled);
			throw interrupt_canceled(0);
		}
		else
		{
			cur->cancel();
			if (ccf_unlikely(global_debug_file))
				LOG_DEBUG("[Logic] [any_of]  %u-<%s> is canceled",
					cur->_unique_id, dst_to_tips(cur));
		}
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
