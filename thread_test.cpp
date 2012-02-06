#include <pthread.h>

#include "test_unit.hpp"
#include "test_util.hpp"
#include "callback.hpp"
#include "thread.hpp"

namespace {

using base::Callback;
using test::TestThread;
using base::makeCallableOnce;
using base::makeCallableMany;

TEST(Single, BitFlip) {
  TestThread test;

  Callback<void>* thread_method = makeCallableOnce(&TestThread::hit, &test);
 
  pthread_t my_thread = makeThread(thread_method);

  pthread_join(my_thread, NULL);
  
  EXPECT_TRUE(test.is_hit);
}

} // unnammed namespace

int main(int argc, char* argv[]) {
  return RUN_TESTS(argc, argv);
}
