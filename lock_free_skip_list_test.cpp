#define LOOP_COUNT 100

#include <pthread.h>  // barriers

#include "callback.hpp"
#include "lock_free_skip_list.cpp"
#include "op_generator.hpp"
#include "test_unit.hpp"
#include "thread.hpp"

using base::Callback;
using base::makeCallableMany;
using base::makeThread;
using lock_free::LockFreeSkipList;
using lock_free::OpGenerator;

namespace {

struct Tester {
  void SkipListTesterBasic(int start, LockFreeSkipList* skip_list) {
    for (int i = start; i < start + LOOP_COUNT; i++)
      skip_list->Add(i);
  }

  void SkipListTesterOpGen(int* ops[], int op_count) {

  }
};

TEST(Simple, SingleThreaded) {
  LockFreeSkipList skip_list;

  skip_list.Add(3);
  skip_list.Add(10);

  EXPECT_TRUE(skip_list.Contains(3));
  EXPECT_TRUE(skip_list.Contains(10));
  EXPECT_FALSE(skip_list.Contains(5));

  skip_list.Remove(3);
  skip_list.Add(13);

  EXPECT_FALSE(skip_list.Contains(3));
  EXPECT_TRUE(skip_list.Contains(10));
}

TEST(Complex, MultiThreadedLinear) {
  LockFreeSkipList skip_list;
  pthread_t thread_one;
  pthread_t thread_two;
  pthread_t thread_three;
  Tester tester;

  Callback<void, int, LockFreeSkipList*>* cb = 
        makeCallableMany(&Tester::SkipListTesterBasic, &tester);

  Callback<void>* cb_wrapper_one = 
      makeCallableOnce(&Callback<void, int, LockFreeSkipList*>::operator(), 
                       cb,
                       0, 
                       &skip_list);

  Callback<void>* cb_wrapper_two = 
    makeCallableOnce(&Callback<void, int, LockFreeSkipList*>::operator(), 
                     cb,
                     100, 
                     &skip_list);

    Callback<void>* cb_wrapper_three = 
    makeCallableOnce(&Callback<void, int, LockFreeSkipList*>::operator(), 
                     cb,
                     200, 
                     &skip_list);

  thread_one = makeThread(cb_wrapper_one);
  thread_two = makeThread(cb_wrapper_two);
  thread_three = makeThread(cb_wrapper_three);

  pthread_join(thread_one, NULL);
  pthread_join(thread_two, NULL);
  pthread_join(thread_three, NULL);

  EXPECT_TRUE(skip_list.Contains(3));
  EXPECT_TRUE(skip_list.Contains(10));
  EXPECT_TRUE(skip_list.Contains(75));
  EXPECT_TRUE(skip_list.Contains(108));
  EXPECT_TRUE(skip_list.Contains(100));
  EXPECT_TRUE(skip_list.Contains(175));
  EXPECT_TRUE(skip_list.Contains(275));
  EXPECT_FALSE(skip_list.Contains(302));

  delete cb;
}

// TEST(Complex, MultiThreadedMixed) {
//   LockFreeSkipList skip_list;
//   pthread_t thread_one;
//   pthread_t thread_two;
//   Tester tester;

//   int ops_one[200];
//   int ops_two[200];




// }

}

int main(int argc, char* argv[]) {
  return RUN_TESTS(argc, argv);
}