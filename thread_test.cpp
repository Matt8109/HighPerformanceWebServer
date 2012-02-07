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
  EXPECT_EQ(test.count, 0);
}

TEST(Multiple, Count) {
  const int count = 1; // The number of threads to run
  TestThread test;
  pthread_t threads[count];

  Callback<void>* thread_method_one = 
      makeCallableMany(&TestThread::increase, &test);

  for (int i = 0; i < count; i++)
    threads[count] = makeThread(thread_method_one);

  for (int i = 0; i < count; i++)
    pthread_join(threads[count], NULL);

  delete thread_method_one; // Delete callback after all threads finish

  EXPECT_EQ(test.count, count);  
}
    


} // unnammed namespace

int main(int argc, char* argv[]) {
  return RUN_TESTS(argc, argv);
}
