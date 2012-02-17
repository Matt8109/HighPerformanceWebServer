#include "thread_pool_fast.hpp"

namespace base {

ThreadPoolFast::ThreadPoolFast(int num_workers) 
    : status_(IS_RUNNING),
		  stop_count_(0),
		 	thread_count_(num_workers),
			active_thread_count_(num_workers)	{
  thread_method_ = makeCallableMany(&ThreadPoolFast::ThreadMethod, this);	
	
	for (int i = 0; i < thread_count_; i++)
	  thread_list_.push_back(makeThread(thread_method_));
}

ThreadPoolFast::~ThreadPoolFast() {
	delete thread_method_;
}

void ThreadPoolFast::stop() {
	pthread_t current_thread_ = pthread_self();

	struct timespec timeout;
  timeout.tv_sec = 0;
	timeout.tv_nsec = 300;

	sync_root_.lock();
	// check to see if we are actually a thread in the pool
	for (int i = 0; i < thread_count_; i++) {
		if (thread_list_[i] == current_thread_) 
			stop_count_++;	// let others know we are waiting if a pool thread
	}

	if (status_ == IS_STOPPING) { // someone has already called stop
		//now we have to wait for the other thread to finish shutting down the pool
		while (status_ != IS_STOPPED)
	  	not_empty_.timedWait(&sync_root_, &timeout);		

		// ok the other thread has finished stopping the pool, we can exit
		stop_count_--; // not needed, but might help debug

		sync_root_.unlock();
		return;
	}

  status_ = IS_STOPPING; // from now on no new tasks will be accepted
	sync_root_.unlock();

	while (task_queue_.size() != 0)
		usleep(50000); // 50 ms

	// try to join on all threads
	for (int i = 0; i < thread_count_; i++)
		pthread_tryjoin_np(thread_list_[i], NULL);

	sync_root_.lock();
	
	// wait until all threads not calling stop have completed
	while (active_thread_count_ >  stop_count_)
		not_empty_.timedWait(&sync_root_, &timeout);		

	status_ = IS_STOPPED;
	sync_root_.unlock();	
}

void ThreadPoolFast::addTask(Callback<void>* task) {
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

int ThreadPoolFast::count() const {
	return task_queue_.size();
}

bool ThreadPoolFast::isStopped() {
		return status_ == IS_STOPPED ? true : false;
}

void ThreadPoolFast::ThreadMethod() {
	Callback<void>* cb;
  struct timespec timeout;
  timeout.tv_sec = 0;
	timeout.tv_nsec = 300;

	while (true) {
		sync_root_.lock();

		while (task_queue_.size() == 0 && status_ == IS_RUNNING)
	  	not_empty_.timedWait(&sync_root_, &timeout);		

		if (task_queue_.size() == 0 && status_ != IS_RUNNING) { 
			active_thread_count_--; // helps the stop call
			sync_root_.unlock(); // if the pool is stopped, and queue empty
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
