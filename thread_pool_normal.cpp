// Code copyright Alberto Lerner and Matthew Mancuso
// See git blame for details

#include "thread_pool_normal.hpp"

namespace base {

ThreadPoolNormal::ThreadPoolNormal(int num_workers) 
    : status_(IS_RUNNING),
	  stop_count_(0),
	  thread_count_(num_workers),
	  active_thread_count_(num_workers)	{
  thread_method_ = makeCallableMany(&ThreadPoolNormal::ThreadMethod, this);	
	
	for (int i = 0; i < thread_count_; i++)
	  thread_list_.push_back(makeThread(thread_method_));
}

ThreadPoolNormal::~ThreadPoolNormal() {
	delete thread_method_;
}

void ThreadPoolNormal::stop() {
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

void ThreadPoolNormal::addTask(Callback<void>* task) {
	if (status_ == IS_RUNNING) { // the pool is still running
		sync_root_.lock(); //making internal changes
	
		task_queue_.push(task);

		sync_root_.unlock();

		not_empty_.signal(); // alert the threads
	}
}

int ThreadPoolNormal::count() const {
	return task_queue_.size();
}

bool ThreadPoolNormal::isStopped() {
		return status_ == IS_STOPPED ? true : false;
}

void ThreadPoolNormal::ThreadMethod() {
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
	}
}

} // namespace base
