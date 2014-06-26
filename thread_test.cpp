// Code copyright Alberto Lerner and Matthew Mancuso
// See git blame for details

#include <pthread.h>

#include "test_unit.hpp"
#include "test_util.hpp"
#include "callback.hpp"
#include "thread.hpp"

namespace {

using base::Callback;
using test::TestClass;
using base::makeCallableOnce;
using base::makeCallableMany;

TEST(Single, BitFlip) {
  TestClass test;

  Callback<void>* thread_method = makeCallableOnce(&TestClass::hit, &test);
 
  pthread_t my_thread = makeThread(thread_method);

  pthread_join(my_thread, NULL);
  
  EXPECT_TRUE(test.is_hit);
  EXPECT_EQ(test.count, 0);
}

TEST(Single, Count) {
  TestClass test;

  Callback<void>* thread_method = 
		makeCallableOnce(&TestClass::increase, &test);

	EXPECT_TRUE(thread_method->once());
 
  pthread_t my_thread = makeThread(thread_method);

  pthread_join(my_thread, NULL);

  EXPECT_EQ(test.count, 1);
  EXPECT_FALSE(test.is_hit);
}

TEST(Multiple, Count) {
  const int count = 8; // The number of threads to run
  TestClass test;
  pthread_t threads[count];

  Callback<void>* thread_method_one = 
      makeCallableMany(&TestClass::increase, &test);

	EXPECT_FALSE(thread_method_one->once());

  for (int i = 0; i < count; i++)
    threads[count] = makeThread(thread_method_one);

  for (int i = 0; i < count; i++)
    pthread_join(threads[count], NULL);

  EXPECT_EQ(test.count, count);  

  delete thread_method_one;
}
    
} // unnammed namespace

int main(int argc, char* argv[]) {
  return RUN_TESTS(argc, argv);
}
