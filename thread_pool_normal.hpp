#ifndef MCP_BASE_THREAD_POOL_NORMAL_HEADER
#define MCP_BASE_THREAD_POOL_NORMAL_HEADER

#include "callback.hpp"
#include "thread_pool.hpp"

namespace base {

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
