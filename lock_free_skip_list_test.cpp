#define LOOP_COUNT 100
#define THREAD_COUNT 5

#include <algorithm>
#include <pthread.h>

#include "callback.hpp"
#include "lock_free_skip_list.cpp"
#include "test_unit.hpp"
#include "thread.hpp"

using base::Callback;
using base::makeCallableMany;
using base::makeThread;
using lock_free::LockFreeSkipList;
using std::random_shuffle;

namespace {

struct Tester {
  LockFreeSkipList* list_;

  Tester(LockFreeSkipList* skip_list) : list_(skip_list) { }
  ~Tester() { }

  void SkipListTesterBasic(int start) {
    for (int i = start; i < start + LOOP_COUNT; i++)
      list_->Add(i);
  }

  void SkipListTesterRandom(int start, int* ops) {
    //std::cout << "Printing " << start << " - " << start + LOOP_COUNT << std::endl << std::flush;
    for (int i = start; i < start + LOOP_COUNT; i++)
      list_->Add(ops[i]);
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
  Tester tester(&skip_list);

  Callback<void, int>* cb = 
        makeCallableMany(&Tester::SkipListTesterBasic, &tester);

  Callback<void>* cb_wrapper_one = 
      makeCallableOnce(&Callback<void, int>::operator(), 
                       cb,
                       0);

  Callback<void>* cb_wrapper_two = 
    makeCallableOnce(&Callback<void, int>::operator(), 
                     cb,
                     100);

    Callback<void>* cb_wrapper_three = 
    makeCallableOnce(&Callback<void, int>::operator(), 
                     cb,
                     200);

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

TEST(Complex, MultiThreadMixDuplicates) {
  LockFreeSkipList skip_list;
  pthread_t threads[THREAD_COUNT];
  Tester tester(&skip_list);

  int values[LOOP_COUNT * THREAD_COUNT];

  for (int i = 0; i < LOOP_COUNT * THREAD_COUNT; i++)
    values[i] = i;

  random_shuffle(values, values + LOOP_COUNT * THREAD_COUNT);

  Callback<void, int, int*>* cb = 
        makeCallableMany(&Tester::SkipListTesterRandom, &tester);

  for (int i = 0; i < THREAD_COUNT; i++) {
    Callback<void>* cb_wrapper = 
        makeCallableOnce(&Callback<void, int, int*>::operator(), 
                         cb,
                         i * LOOP_COUNT,
                         values);

    threads[i] = makeThread(cb_wrapper);
  }

  for (int i = 0; i < THREAD_COUNT; i++)
    pthread_join(threads[i], NULL);

  for (int i = 0; i < LOOP_COUNT * THREAD_COUNT; i++) {
    EXPECT_TRUE(skip_list.Contains(i));

    if (!skip_list.Contains(i))
      std::cout << i << std::endl;
  }

  delete cb;
}

} // unamed namespace

int main(int argc, char* argv[]) {
  return RUN_TESTS(argc, argv);
}