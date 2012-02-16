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
	pthread_t current_thread_ = pthread_self();

	sync_root_.lock();
	if (status_ == IS_STOPPING) { // someone has already called stop
		sync_root_.unlock();
		return;
	}

  status_ = IS_STOPPING; // from now on no new tasks will be accepted
	sync_root_.unlock();

	while (task_queue_.size() != 0)
		usleep(50000); // 50 ms

	// at this point the queue is empty, nothing more can be added, so we just
	// need to wait on all the threads to finish
  for (int i = 0; i < thread_count_; i++) {
		if (thread_list_[i] != current_thread_) { // we dont want to call join on us
			pthread_join(thread_list_[i], NULL);
		}
	}

  // ok once we are here, we are done
  sync_root_.lock();
	status_ = IS_STOPPED;
	sync_root_.unlock();	
}

void ThreadPoolNormal::addTask(Callback<void>* task) {
	if (status_ == IS_RUNNING) { // the pool is still running
		sync_root_.lock(); //making internal changes

		if (!task->once())
			delete_list_[task] = delete_list_[task]++; // increase our reference count
	
		task_queue_.push(task);

		sync_root_.unlock();

		not_empty_.signal(); // alert the threads
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
	return task_queue_.size();
}

void ThreadPoolNormal::ThreadMethod() {
	Callback<void>* cb;
  struct timespec timeout;
  timeout.tv_sec = 0;
	timeout.tv_nsec = 100;

	while (true) {
		sync_root_.lock();

		while (task_queue_.size() == 0 && status_ == IS_RUNNING)
	  	not_empty_.timedWait(&sync_root_, &timeout);		

		if (task_queue_.size() == 0 && status_ != IS_RUNNING) { // if the pool is
			sync_root_.unlock();																	// stopped and the queue empty
			return;
		}

		cb = task_queue_.front();
		task_queue_.pop();

		sync_root_.unlock();
	
		(*cb)(); // execute the task

		if (!cb) { // if the callback is used multiple times
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
