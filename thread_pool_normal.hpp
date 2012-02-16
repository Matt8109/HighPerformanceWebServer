#ifndef MCP_BASE_THREAD_POOL_NORMAL_HEADER
#define MCP_BASE_THREAD_POOL_NORMAL_HEADER

#define IS_RUNNING 0
#define IS_STOPPING 1
#define IS_STOPPED 2

#include <map>
#include <vector>
#include <pthread.h>

#include "lock.hpp"
#include "callback.hpp"
#include "thread_pool.hpp"
#include "safe_queue.hpp"

namespace base {
using base::Mutex;
using base::SafeQueue;
using std::vector;
using std::map;

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

	int status_; //the status of the pool, IS_RUNNING, IS_STOPPING, IS_STOPPED
	int thread_count_; //the number of threads in the thread pool
	Mutex sync_root_; // for syn
	pthread_cond_t not_empty_; // wait condition, signal when the queue gets a new item
	vector<pthread_t> thread_list_; // holds the collection of threads in the pool
	Callback<void>* thread_method_; // the callback for the the method that runs tasks
	SafeQueue<Callback<void>*> task_queue_; // the queue of tasks to be completed
  map<Callback<void>*, int> delete_list_;
};

} // namespace base

#endif // MCP_BASE_THREAD_POOL_NORMAL_HEADER
