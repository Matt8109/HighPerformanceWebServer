#include "thread_pool.hpp"

namespace base {
	ThreadPool::ThreadPool(int num_workers) 
	    : thread_count_(num_workers) {
	  thread_list_ = new pthread_t[num_workers];
	}

	ThreadPool::~ThreadPool() {
		delete thread_list_;
	}
}	
