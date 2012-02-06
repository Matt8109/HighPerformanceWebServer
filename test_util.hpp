#ifndef MCP_BASE_TEST_UTIL_HEADER
#define MCP_BASE_TEST_UTIL_HEADER

#include "lock.hpp"

namespace test {

using base::Mutex;

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

  bool between(int i, int j) {
    if (i > count_) return false;
    if (j < count_) return false;
    return true;
  }

private:
  int  count_;
};
    
// A simple class for testing threads
class TestThread {
public:
    int count;
    bool is_hit;
    Mutex sync_root;
    
    TestThread()  
        : count(0),
          is_hit(false) {
    }
    
    void hit() {
        is_hit = true;
    }
    
    void increase() {
        sync_root.lock();
        count++;
        sync_root.unlock();
    }
};

}  // namespace base

#endif // MCP_BASE_TEST_UTIL_HEADER
