#include <pthread.h>  // barriers

#include "callback.hpp"
#include "lock_free_skip_list.cpp"
#include "test_unit.hpp"

namespace {
  TEST(Basic, SimpleOperations) {
    lock_free::LockFreeSkipList test(5);

    test.Add(3);
    test.Add(10);

    EXPECT_TRUE(test.Contains(3));
    EXPECT_TRUE(test.Contains(10));
    EXPECT_FALSE(test.Contains(5));

    test.Remove(3);
    EXPECT_FALSE(test.Contains(3));
    EXPECT_TRUE(test.Contains(10));
  }
}

int main(int argc, char *argv[]) {
  return RUN_TESTS(argc,argv);
}