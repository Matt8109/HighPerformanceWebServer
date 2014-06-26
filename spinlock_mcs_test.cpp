// Code copyright Alberto Lerner and Matthew Mancuso
// See git blame for details

#include <unistd.h>

#include "callback.hpp"
#include "logging.hpp"
#include "spinlock_mcs.hpp"
#include "test_unit.hpp"
#include "thread.hpp"

namespace {

using base::Callback;
using base::makeCallableOnce;
using base::makeThread;
using base::SpinlockMcs;

const int CORE_COUNT = sysconf( _SC_NPROCESSORS_ONLN );

// ************************************************************
// Support for concurrent test
//

class LockTester {
public:
  LockTester(SpinlockMcs* spin, int* counter);
  ~LockTester() { }

  void start(int increments);
  void join();

  int requests() const { return requests_; }

private:
  SpinlockMcs* spin_;
  int*      counter_;
  int       requests_;
  pthread_t tid_;

  void test(int increments);

  // Non-copyable, non-assignable
  LockTester(LockTester&);
  LockTester& operator=(LockTester&);
};

LockTester::LockTester(SpinlockMcs* spin, int* counter)
  : spin_(spin), counter_(counter), requests_(0) {
}

void LockTester::start(int increments) {
  Callback<void>* body = makeCallableOnce(&LockTester::test, this, increments);
  tid_ = makeThread(body);
}

void LockTester::join() {
  pthread_join(tid_, NULL);
}

void LockTester::test(int increments) {
  while (increments-- > 0) {
    spin_->lock();
    ++(*counter_);
    ++requests_;
    spin_->unlock();
  }
}

// ************************************************************
// Test cases
//

TEST(Basic, LockUnlock) {
  SpinlockMcs spinlock;

  spinlock.lock();
  spinlock.unlock();

  EXPECT_TRUE(true);
}

TEST(Concurrency, counters) {
  SpinlockMcs spin;
  int counter = 0;

  const int threads = CORE_COUNT;
  const int incs = 500000;
  LockTester* testers[threads];

  for (int i = 0; i < threads; i++) {
    testers[i] = new LockTester(&spin, &counter);
  }
  for (int i = 0; i < threads; i++) {
    testers[i]->start(incs);
  }
  for (int i = 0; i < threads; i++) {
    testers[i]->join();
    EXPECT_EQ(testers[i]->requests(), incs);
    delete testers[i];
  }

  EXPECT_EQ(counter, threads*incs);
}

}  // unnamed namespace

int main(int argc, char *argv[]) {
  return RUN_TESTS(argc, argv);
}
