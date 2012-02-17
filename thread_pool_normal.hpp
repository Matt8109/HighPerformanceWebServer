#ifndef MCP_BASE_THREAD_POOL_NORMAL_HEADER
#define MCP_BASE_THREAD_POOL_NORMAL_HEADER

#define IS_RUNNING 0
#define IS_STOPPING 1
#define IS_STOPPED 2

#include <time.h>
#include <vector>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include "lock.hpp"
#include "callback.hpp"
#include "thread_pool.hpp"

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
	virtual bool isStopped();

private:
  // Non-copyable, non-assignable.
  ThreadPoolNormal(const ThreadPoolNormal&);
  ThreadPoolNormal& operator=(const ThreadPoolNormal&);

	void ThreadMethod();

	int status_; //the status of the pool, IS_RUNNING, IS_STOPPING, IS_STOPPED
	int stop_count_; // the number of threads waiting on a stop
	int thread_count_; //the number of threads in the thread pool
	int active_thread_count_; // number of thread currently running
	Mutex sync_root_; // for syn
	ConditionVar not_empty_; //for waking up threads
	vector<pthread_t> thread_list_; // holds the collection of threads in the pool
	Callback<void>* thread_method_; // the callback for the the method that runs tasks
	mutable queue<Callback<void>*> task_queue_; // the queue of tasks to be completed
};

} // namespace base

#endif // MCP_BASE_THREAD_POOL_NORMAL_HEADER
