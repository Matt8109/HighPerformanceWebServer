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
  ConditionVar while_not_empty_;

	sync_root_.lock();
  status_ = IS_STOPPING; // from now on no new tasks will be accepted
	sync_root_.unlock();

	while (task_queue_.count() != 0)
		while_not_empty_.timedWait(

}

void ThreadPoolNormal::addTask(Callback<void>* :task) {
	if (status_ == IS_RUNNING) { // the pool is still running
		sync_root_.lock(); //making internal changes

		if (!task->once())
			delete_list_[task] = delete_list_[task]++; // increase our reference count
		
		task_queue_.push(task);
		sync_root_.unlock();

		pthread_cond_signal(&not_empty_); // alert the threads
	}
	else if (status_ == IS_STOPPING) { // in the process of shutting
		if (!task->once()) { // we need to delete the task, but it might be running
		}
	} else {
		if (!task->once())
			delete task; // safe to delete, the pool is stopped
	}
}

int ThreadPoolNormal::count() const {
	task_queue.size();
}

void ThreadPoolNormal::ThreadMethod() {
	Callback<void>* cb;

	while (true) {
		sync_root_.lock();

		while (task_queue_.count() == 0 && status_ != IS_STOPPED)
			pthread_cond_wait(&not_empty_, &sync_root_);

		if (status_ == IS_STOPPED) // if the pool is stopped we can exit out
			return; 

		cb = task_queue_.pop();

		sync_root.unlock();
	
		(*cb)(); // execute the task

		if (!cb->once()) { // if the callback is used multiple times
			int ref_count;   // the reference count for the callback

			sync_root_.lock();
			delete_list_[cb] = delete_list_[cb]--;  // decrease reference count
			ref_count = delete_list_[cb];

			if (ref_count == 0)  // if this was the last reference to the callback
				delete cb;         // then it is safe to delete
					
			sync_root_.unlock();
		}
	}
}

} // namespace base
