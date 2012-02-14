#ifndef MCP_BASE_THREAD_POOL_NORMAL_HEADER
#define MCP_BASE_THREAD_POOL_NORMAL_HEADER

#include <pthread.h>

#include "callback.hpp"
#include "thread_pool.hpp"
#include "safe_queue.hpp"

namespace base {
using base::Mutex;
using base::SafeQueue;

class ThreadPoolNormal : public ThreadPool {
public:
  // ThreadPoolNormal interface
  explicit ThreadPoolNormal(int num_workers);
  virtual ~ThreadPoolNormal();

  virtual void addTask(Callback<void>* task);
  virtual void stop();
  virtual int count() const;

private:
  // Non-copyable, non-assignable.
  ThreadPoolNormal(const ThreadPoolNormal&);
  ThreadPoolNormal& operator=(const ThreadPoolNormal&);
};

} // namespace base

#endif // MCP_BASE_THREAD_POOL_NORMAL_HEADER
