#include <pthread.h>

#include "test_unit.hpp"
#include "thread.hpp"
#include "lock.hpp"

namespace {
using base::Mutex;

TEST(Single, BitFlip) {
  TestThread test;

  
  EXPECT_TRUE(true);
}

struct TestThread {
  int count;
  bool hit;
  Mutex sync_root;

  TestThread() {
    count = 0;
    hit = false;
  }

  void Hit() {
    hit = true;
  }

  void Increase() {
    sync_root.lock();
    count++;
    sync_root.unlock();
  }
};

} // unnammed namespace

int main(int argc, char* argv[]) {
  return RUN_TESTS(argc, argv);
}
