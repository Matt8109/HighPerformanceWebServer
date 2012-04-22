#include <pthread.h>  // barriers

#include "callback.hpp"
#include "test_unit.hpp"

namespace {
  TEST(Basic, SimpleInsertion) {
    EXPECT_TRUE(true);
  }
}

int main(int argc, char *argv[]) {
  return RUN_TESTS(argc,argv);
}