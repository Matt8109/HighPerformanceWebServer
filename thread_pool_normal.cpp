#include "thread_pool_normal.hpp"

namespace base {

ThreadPoolNormal::ThreadPoolNormal(int num_workers) {
}

ThreadPoolNormal::~ThreadPoolNormal() {
}

void ThreadPoolNormal::stop() {
}

void ThreadPoolNormal::addTask(Callback<void>* task) {
}

int ThreadPoolNormal::count() const {
  return 0;
}

} // namespace base
