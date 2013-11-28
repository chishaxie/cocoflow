#include "cocoflow-comm.h"

namespace ccf {

struct setting
{
	uint32 stack_size;
	uint32 protect_size;
	uint32 max_task_num;
	setting(uint32 stack_size, uint32 protect_size, uint32 max_task_num)
		: stack_size(stack_size), protect_size(protect_size), max_task_num(max_task_num)
	{}
};

/* extern in cocoflow.h */
bool          global_initialized = false;
event_task**  global_task_manager = NULL;

/* extern in cocoflow-comm.h */
coroutine*    global_running_manager = NULL;
coroutine     global_loop_running;
event_task*   global_current_task = NULL;
bool          global_signal_canceled = false;
FILE*         global_debug_file = NULL;

static std::list<setting> setting_list;
static uint32 g_max_task_num = 0;
static size_t g_max_stack_size = 0;
static event_task* top_task;

void set_debug(FILE* fp)
{
	global_debug_file = fp;
}

inline void __task_start_child(event_task* cur, event_task* child)
{
	swap_running(cur->_unique_id, child->_unique_id);
}

inline void __task_cancel_children(event_task* cur, event_task** children, uint32 num)
{
	uint32 unsupport = 0;
	for (uint32 i=0; i<num; i++)
	{
		event_task* child = children[i];
		if (child->_status == running)
		{
			for (event_task* final = child; ; final = final->reuse)
			{
				if (!final->_interruptable)
				{
					if (final != child)
						final->_status = canceled;
					unsupport ++;
					break;
				}
				if (!final->reuse)
				{
					global_signal_canceled = true;
					child->_status = canceled;
					swap_running(cur->_unique_id, child->_unique_id);
					break;
				}
			}
		}
	}
	if (unsupport)
	{
		cur->uninterruptable();
		for (uint32 i=0; i<unsupport; i++)
			CHECK(__task_yield(cur) == true);
	}
}

void __task_runtime(uint32);

void __init_setting(uint32* &free_list_front, uint32* &free_list_end, uint32 stack_size, uint32 protect_size, uint32 max_task_num)
{
	free_list_front = new uint32[max_task_num];
	free_list_end = free_list_front + max_task_num;
	for (uint32 i=0; i<max_task_num; i++, g_max_task_num++)
		free_list_front[i] = g_max_task_num;
	g_max_stack_size += stack_size * max_task_num;
	setting_list.push_back(setting(stack_size, protect_size, max_task_num));
}

void __init()
{
	global_initialized = true;
	
	void* mem = coroutine_memory_alloc(g_max_stack_size);
	if (!mem)
		FATAL_ERROR("Out of memory");
	
	global_task_manager = new event_task*[g_max_task_num];
	global_running_manager = new coroutine[g_max_task_num];
	
	uint32 unique_id = 0;
	void* cur_mem = mem;
	for (std::list<setting>::iterator it = setting_list.begin(); it != setting_list.end(); it++)
	{
		for (uint32 i=0; i<it->max_task_num; i++)
		{
			if (it->protect_size)
				coroutine_memory_protect(cur_mem, it->protect_size);
			coroutine_create(&global_running_manager[unique_id], cur_mem, it->stack_size, unique_id);
			unique_id ++;
			cur_mem = (void*)((char*)cur_mem + it->stack_size);
		}
	}
}

void __task_runtime(uint32 _unique_id)
{
	for (;;)
	{
		event_task* this_task = global_task_manager[_unique_id];
		
		this_task->_status = running;
		try {
			this_task->run();
		} catch (interrupt_canceled& sig) {
			this_task->_status = canceled;
			this_task->cancel();
		}
		if (this_task->_status == running)
			this_task->_status = completed;
			
		uint32 next;
		if (this_task->finish_to != EVENT_LOOP_ID || this_task->block_to == EVENT_LOOP_ID)
			next = this_task->finish_to; //End of await or start
		else
			next = this_task->block_to; //End of start without really block
			
		if (this_task->finish_to == EVENT_LOOP_ID && this_task != top_task)
			delete this_task;
		
		swap_running(_unique_id, next);
	}
}

int __await(event_task* target)
{
	if (ccf_unlikely(!global_current_task))
		FATAL_ERROR("Call await() must be in a task");
	
	if (ccf_unlikely(target->_status != ready && target->_status != limited))
		return -1;
	
	event_task* parent = global_current_task;
	
	//Passing parent's block to child's block
	target->block_to = parent->block_to;
	parent->block_to = EVENT_LOOP_ID;
	
	target->finish_to = parent->_unique_id;
	target->_unique_id = parent->_unique_id;
	
	target->_status = running;
	global_current_task = target;
	parent->reuse = target;
	try {
		target->run();
	} catch (interrupt_canceled& sig) {
		target->_status = canceled;
		target->cancel();
		
		//Must be careful
		global_current_task = parent;
		parent->reuse = NULL;
		target->_unique_id = EVENT_LOOP_ID;
		
		throw interrupt_canceled(sig.level + 1);
	}
	global_current_task = parent;
	parent->reuse = NULL;
	if (target->_status == running)
		target->_status = completed;
		
	target->_unique_id = EVENT_LOOP_ID;
	
	//Passing child's block to parent's block cause child without really block
	if (target->block_to != EVENT_LOOP_ID)
	{
		parent->block_to = target->block_to;
		target->block_to = EVENT_LOOP_ID;
	}
	
	if (target->_status == canceled) //Only unsupport cancel can reach
	{
		target->_status = completed;
		throw interrupt_canceled(0);
	}
	
	return 0;
}

int __start(event_task* target)
{
	if (ccf_unlikely(!global_current_task))
		FATAL_ERROR("Call start() must be in a task");
	
	if (ccf_unlikely(target->_status != ready))
	{
		delete target;
		return -1;
	}
	
	event_task* parent = global_current_task;
	
	target->block_to = parent->_unique_id;
	target->finish_to = EVENT_LOOP_ID;
	
	swap_running(parent->_unique_id, target->_unique_id);
	
	return 0;
}

void __cocoflow(event_task* top)
{
	static int call = 0;
	CHECK((++call) == 1);
	
	top_task = top;
	
	top_task->block_to = EVENT_LOOP_ID;
	top_task->finish_to = EVENT_LOOP_ID;
	
	coroutine_by_thread(&global_loop_running);
	swap_running(EVENT_LOOP_ID, top_task->_unique_id);
	
	(void)uv_run(loop(), UV_RUN_DEFAULT);
}

/***** all_of *****/

all_of::all_of(event_task* targets[], uint32 num) : num(num)
{
	CHECK(this->num != 0);
	this->children = targets;
}

void all_of::run()
{
	for (uint32 i=0; i<this->num; i++)
	{
		if (ccf_unlikely(this->children[i]->_status != ready))
		{
			this->_status = child_unready;
			return;
		}
		this->children[i]->block_to = this->_unique_id;
		this->children[i]->finish_to = this->_unique_id;
	}
	
	for (uint32 i=0; i<this->num; i++)
		__task_start_child(this, this->children[i]);
		
	for (;;)
	{
		bool all_completed = true;
		for (uint32 i=0; i<this->num; i++)
			if (this->children[i]->_status != completed)
				all_completed = false;
		if (all_completed)
			break;
		if (!__task_yield(this))
			return;
	}
}

void all_of::cancel()
{
	__task_cancel_children(this, this->children, this->num);
}

all_of::~all_of()
{
}

/***** any_of *****/

any_of::any_of(event_task* targets[], uint32 num) : num(num), completed_id(-1)
{
	CHECK(this->num != 0);
	this->children = targets;
}

int any_of::who_completed()
{
	return this->completed_id;
}

void any_of::run()
{
	for (uint32 i=0; i<this->num; i++)
	{
		if (ccf_unlikely(this->children[i]->_status != ready))
		{
			this->_status = child_unready;
			return;
		}
		this->children[i]->block_to = this->_unique_id;
		this->children[i]->finish_to = this->_unique_id;
	}
	
	for (uint32 i=0; i<this->num; i++)
	{
		__task_start_child(this, this->children[i]);
		if (this->children[i]->_status == completed)
			break;
	}
		
	for (;;)
	{
		for (uint32 i=0; i<this->num; i++)
			if (this->children[i]->_status == completed)
				this->completed_id = i;
		if (this->completed_id != -1)
			break;
		if (!__task_yield(this))
			return;
	}
	
	__task_cancel_children(this, this->children, this->num);
}

void any_of::cancel()
{
	__task_cancel_children(this, this->children, this->num);
}

any_of::~any_of()
{
}

/***** uv *****/

void free_self_close_cb(uv_handle_t* handle)
{
	free(handle);
}

}
