#include "thread_pool_fast.hpp"

namespace base {

using base::Callback;
using base::makeCallableOnce;

//
//  ThreadPoolFast Definitions
//

ThreadPoolFast::ThreadPoolFast(int num_workers) 
    : ThreadPool(num_workers) {
}

ThreadPoolFast::~ThreadPoolFast() {
}

void ThreadPoolFast::stop() {
}

void ThreadPoolFast::addTask(Callback<void>* task) {
}

int ThreadPoolFast::count() const {
  return 0;
}

bool ThreadPoolFast::isStopped() {
	return false;
}

} // namespace base
