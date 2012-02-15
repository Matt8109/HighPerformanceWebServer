#ifndef MCP_BASE_THREAD_POOL_HEADER
#define MCP_BASE_THREAD_POOL_HEADER

#include <list>
#include <pthread.h>

#include "lock.hpp"
#include "callback.hpp"
#include "safe_queue.hpp"

namespace base {

struct ThreadWrapper;

// Abstract base class for experimenting with thread-pool strategies.
class ThreadPool {
public:
  // Cleans up any pending callbacks that weren't executed. Pending
  // callbacks may have been added after stop() was called if a running
  // worker issued new addTask()s.
  //
  // REQUIRES: stop() have completed executing.
  virtual ~ThreadPool(); 

  // Requests the execution of 'task' on an undetermined worker thread.
  virtual void addTask(Callback<void>* task) = 0;

  // Waits for all the workers to finish processing the pending tasks
  // and stop then stop the pool. This call may be issued from within
  // a worker thread itself.
  virtual void stop() = 0;

  // accessors

  // Returns the current size of the dispatch queue (pending tasks).
  virtual int count() const = 0;


protected:
	// Needed to create the correct number of threads for the pool
	ThreadPool(int num_workers);

	bool stopping_; //if the thread pool is stopped, or processing stop
	int thread_count_;
	Mutex sync_root_; //for general locking
  std::list<ThreadWrapper> thread_list_;
	SafeQueue<Callback<void>*> pending_tasks_;
};

struct ThreadWrapper {
public:
	pthread_t thread_;
	bool is_running_;
	Callback<void>* current_task_;  

	ThreadWrapper(pthread_t thread) 
		  : thread_(thread) {
  }

	~ThreadWrapper() { }
};

} // namespace base

#endif // MCP_BASE_THREAD_POOL_HEADER
