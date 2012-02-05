#ifndef MCP_BASE_THREAD_POOL_FAST_HEADER
#define MCP_BASE_THREAD_POOL_FAST_HEADER

#include "callback.hpp"
#include "thread_pool.hpp"

namespace base {

class ThreadPoolFast : public ThreadPool {
public:

  // ThreadPool interface
  explicit ThreadPoolFast(int num_workers);
  virtual ~ThreadPoolFast();

  virtual void addTask(Callback<void>* task);
  virtual void stop();
  virtual int count() const;

private:

  // Non-copyable, non-assignable.
  ThreadPoolFast(const ThreadPoolFast&);
  ThreadPoolFast& operator=(const ThreadPoolFast&);
};

} // namespace base

#endif // MCP_BASE_THREAD_POOL_FAST_HEADER
