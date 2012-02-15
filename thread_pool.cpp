#include "thread_pool.hpp"

namespace base {
	ThreadPool::ThreadPool(int num_workers) 
	    : thread_count_(num_workers) {
	  thread_list_ = new ThreadWrapper[num_workers];
	}

	ThreadPool::~ThreadPool() {
		delete thread_list_;
	}
}	
