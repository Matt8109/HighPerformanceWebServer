// Code copyright Alberto Lerner and Matthew Mancuso
// See git blame for details

#define LOOP_COUNT 100
#define THREAD_COUNT 5

#include <algorithm>
#include <pthread.h>

#include "callback.hpp"
#include "lock_free_skip_list.hpp"
#include "test_unit.hpp"
#include "thread.hpp"

using base::Callback;
using base::makeCallableMany;
using base::makeThread;
using lock_free::LockFreeSkipList;
using std::random_shuffle;

namespace {

struct Tester {
  LockFreeSkipList<int>* list_;

  Tester(LockFreeSkipList<int>* skip_list) : list_(skip_list) { }
  ~Tester() { }

  void SkipListTesterBasic(int start) {
    for (int i = start; i < start + LOOP_COUNT; i++)
      list_->add(i, 1);
  }

  void SkipListTesterRandom(int start, int* ops) {
    for (int i = start; i < start + LOOP_COUNT; i++)
      list_->add(ops[i], 0);
  }

  void SkipListTesterRandomDelete(int start, int* ops) {
    for (int i = start; i < start + LOOP_COUNT; i++)
      list_->remove(ops[i]);
  }
};

TEST(Simple, SingleThreaded) {
  LockFreeSkipList<int> skip_list;

  skip_list.add(3, 5);
  skip_list.add(10, 0);

  EXPECT_TRUE(skip_list.contains(3));
  EXPECT_TRUE(skip_list.contains(10));
  EXPECT_FALSE(skip_list.contains(5));

  EXPECT_EQ(skip_list.get(3), 5);

  skip_list.remove(3);
  EXPECT_EQ(skip_list.get(3), 0);

  skip_list.add(13, 0);

  EXPECT_FALSE(skip_list.contains(3));
  EXPECT_TRUE(skip_list.contains(10));
}

TEST(Complex, MultiThreadedLinear) {
  LockFreeSkipList<int> skip_list;
  pthread_t threads[THREAD_COUNT];
  Tester tester(&skip_list);

  Callback<void, int>* cb = 
        makeCallableMany(&Tester::SkipListTesterBasic, &tester);

  for (int i = 0; i < THREAD_COUNT; i++) {
    Callback<void>* cb_wrapper = 
        makeCallableOnce(&Callback<void, int>::operator(), 
                         cb,
                         i * LOOP_COUNT);

    threads[i] = makeThread(cb_wrapper);
  }

  for (int i = 0; i < THREAD_COUNT; i++)
    pthread_join(threads[i], NULL);

  for (int i = 0; i < LOOP_COUNT * THREAD_COUNT; i++)
    EXPECT_TRUE(skip_list.contains(i));

  delete cb;
}

TEST(Complex, MultiThreadMixChanges) {
  LockFreeSkipList<int> skip_list;
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

  for (int i = 0; i < LOOP_COUNT * THREAD_COUNT; i++)
    EXPECT_TRUE(skip_list.contains(i));

  delete cb;
}

// check we can process simultaneous deletes
TEST(Complex, MultiThreadMixDeletes) {
  LockFreeSkipList<int> skip_list;
  pthread_t threads[THREAD_COUNT];
  Tester tester(&skip_list);

  int values[LOOP_COUNT * THREAD_COUNT];

  for (int i = 0; i < LOOP_COUNT * THREAD_COUNT; i++) {
    skip_list.add(i, 0);
    values[i] = i;
  }

  random_shuffle(values, values + LOOP_COUNT * THREAD_COUNT);

  Callback<void, int, int*>* cb = 
        makeCallableMany(&Tester::SkipListTesterRandomDelete, &tester);

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

  for (int i = 0; i < LOOP_COUNT * THREAD_COUNT; i++)  // make sure everything
    EXPECT_FALSE(skip_list.contains(i));               // was removed

  delete cb;
}

} // unamed namespace

int main(int argc, char* argv[]) {
  return RUN_TESTS(argc, argv);
}