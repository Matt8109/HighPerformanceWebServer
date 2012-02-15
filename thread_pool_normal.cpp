#include "thread_pool_normal.hpp"

namespace base {

ThreadPoolNormal::ThreadPoolNormal(int num_workers) 
    : status_(IS_RUNNING),
		 	thread_count_(num_workers) {
  thread_method_ = makeCallableMany(&ThreadPoolNormal::ThreadMethod, this);	
	
	for (int i = 0; i < thread_count_; i++)
	  thread_list_.push_back(makeThread(thread_method_));
}

ThreadPoolNormal::~ThreadPoolNormal() {
	delete thread_method_;
}

void ThreadPoolNormal::stop() {
}

void ThreadPoolNormal::addTask(Callback<void>* task) {
	if (status_ == IS_RUNNING) // the pool is still running
		task_queue_.push(task);
	else if (status_ == IS_STOPPING) { // in the process of shutting
		if (!task->once()) // we need to delete the task, but it might be running
			deletion_queue_.push(task); // so we will do it later
	} else {
		if (!task->once())
			delete task; // safe to delete, the pool is stopped
	}
}

int ThreadPoolNormal::count() const {
  return 0;
}

void ThreadMethod() {
}

} // namespace base
