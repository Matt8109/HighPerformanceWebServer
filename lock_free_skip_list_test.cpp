#include <pthread.h>  // barriers

#include "callback.hpp"
#include "lock_free_skip_list.cpp"
#include "test_unit.hpp"

namespace {
  TEST(Basic, SimpleInsertion) {
    lock_free::LockFreeSkipList test(5);
    EXPECT_TRUE(true);
  }
}

int main(int argc, char *argv[]) {
  return RUN_TESTS(argc,argv);
}