// Code copyright Alberto Lerner and Matthew Mancuso
// See git blame for details

#ifndef MCP_BASE_TEST_UTIL_HEADER
#define MCP_BASE_TEST_UTIL_HEADER

#include "lock.hpp"
#include "thread_pool.hpp"

namespace test {

using base::Mutex;
using base::ThreadPool;

// A simple counter class used often in unit test (and not supposed to
// be used outside *_test.cpp files.

class Counter {
public:
  Counter()          { reset(); }
  ~Counter()         { }

  int  count() const { return count_; }
  void set(int i)    { count_ = i; }
  void reset()       { count_ = 0; }
  void inc()         { ++count_; }
  void incBy(int i)  { count_ += i; }

/*  void incByLoop(int i) {
    for (int x = 0; x < i; x++)
      count_++;
  }*/

  bool between(int i, int j) {
    if (i > count_) return false;
    if (j < count_) return false;
    return true;
  }

private:
  int  count_;
};
    
// A simple class for testing threads
class TestClass {
public:
    int count;
    bool is_hit;
    
    TestClass()  
        : count(0),
          is_hit(false) {
    }

    TestClass(ThreadPool* thread_pool)
        : count(0),
          is_hit(false),
          thread_pool_(thread_pool) {
    }
    
    void hit() {
        is_hit = true;
    }

		void flip() {
			is_hit = !is_hit;
		}
    
    void increase() {
				sync_root.lock();
        count++;
        sync_root.unlock();
    }

		// Slow the counter down, for the benchmark test
		void slowIncrease(int loopCount) {
			for (int i=0; i<loopCount; i++) 
				is_hit = !is_hit; // hopefully something the compiler wont optimize 

			sync_root.lock();
			count++;
			sync_root.unlock();
		}

		void stop() {
			if (thread_pool_)
				thread_pool_->stop();
		}
		
  ThreadPool* thread_pool_;
private:
 Mutex sync_root;
};

}  // namespace base

#endif // MCP_BASE_TEST_UTIL_HEADER
