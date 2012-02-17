#include "thread_pool_fast.hpp"

namespace base {

ThreadPoolFast::ThreadPoolFast(int num_workers) 
    : status_(IS_RUNNING),
		  stop_count_(0),
		 	thread_count_(num_workers),
			active_thread_count_(num_workers),
			free_thread_count_(num_workers),
			pending_task_(NULL)	{
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

  status_ = IS_STOPPING;      // from now on no new tasks will be accepted

	fast_sync_root_.lock();     // wake sleeping threads
	no_fast_task_.signalAll();
	fast_sync_root_.unlock();

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

		// fast thread pool change, essentially dump the task in a shared var
		// and alert a free thread to pick it up
		fast_sync_root_.lock();

		if (free_thread_count_ != 0) {
			
			while (pending_task_ != NULL)
				no_fast_task_.wait(&fast_sync_root_); // wait until pending task is empty

			pending_task_ = task;

			fast_sync_root_.unlock();
			no_fast_task_.signal();
			sync_root_.unlock();

			return;
		} else { // we need to queue
			fast_sync_root_.unlock();
		}

		task_queue_.push(task); // queue the task as we would before

		sync_root_.unlock();

		not_empty_.signal(); // alert the threads
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
		
		fast_sync_root_.lock();

		while (pending_task_ == NULL && status_ == IS_RUNNING 
				&& task_queue_.size() == 0)
			no_fast_task_.timedWait(&fast_sync_root_, &timeout);

			free_thread_count_--;       // mark ourself as being active

		if (pending_task_ != NULL) {
			cb = pending_task_;         // grab the task
			pending_task_ = NULL;       // clear the task for someone else
			no_fast_task_.signalAll();
			fast_sync_root_.unlock();   // give someone else the shot

			(*cb)();                    // execute the callback
		}

		fast_sync_root_.unlock();
		sync_root_.lock();
		fast_sync_root_.lock();

		// we are stopping
		if (status_ != IS_RUNNING && task_queue_.size() == 0) {
				active_thread_count_--;
				fast_sync_root_.unlock();
				sync_root_.unlock();

				return;
    }
		
		//if we got here, means there is probabally something in the queue
		while (true) {
			sync_root_.lock();

			if (task_queue_.size() != 0) { // we have something to do
						cb = task_queue_.front();
						task_queue_.pop();

						sync_root_.unlock();

						(*cb)();
			} else { // the queue is empty, go to sleep
				fast_sync_root_.lock();
				free_thread_count_++;
				fast_sync_root_.unlock();
				sync_root_.unlock();

				break;
			}
		}
	}
}

} // namespace base
