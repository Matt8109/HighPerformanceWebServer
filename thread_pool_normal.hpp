#ifndef MCP_BASE_THREAD_POOL_NORMAL_HEADER
#define MCP_BASE_THREAD_POOL_NORMAL_HEADER

#define IS_RUNNING 0
#define IS_STOPPING 1
#define IS_STOPPED 2

#include <vector>
#include <pthread.h>

#include "callback.hpp"
#include "thread_pool.hpp"
#include "safe_queue.hpp"

namespace base {
using base::Mutex;
using base::SafeQueue;
using std::vector;

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

	void ThreadMethod();

	int status_;
	int thread_count_;
	vector<pthread_t> thread_list_;
	Callback<void>* thread_method_;
	SafeQueue<Callback<void>*> task_queue_;
	SafeQueue<Callback<void>*> deletion_queue_;
};

} // namespace base

#endif // MCP_BASE_THREAD_POOL_NORMAL_HEADER
