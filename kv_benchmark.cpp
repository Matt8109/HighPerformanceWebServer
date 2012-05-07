#include <algorithm>
#include <iostream>
#include <pthread.h>
#include <string>
#include <queue>

#include "lock_free_skip_list.hpp"
#include "test_util.hpp"
#include "thread.hpp"
#include "timer.hpp"

using base::Timer;

namespace {

using base::Callback;
using base::makeCallableMany;
using base::makeThread;
using base::Timer;
using lock_free::LockFreeSkipList;
using std::queue;
using std::random_shuffle;
using std::string;
using test::Counter;

struct Tester {
  LockFreeSkipList<int>* list_;

  Tester(LockFreeSkipList<int>* skip_list) : list_(skip_list) { }
  ~Tester() { }

  void skipListTester(int start, int* ops, int loop_count) {
    for (int i = start; i < start + loop_count; i++)
      list_->add(ops[i], 0);
  }
};

int* createOperations(int count, bool in_order) {
  int* array = new int[count];

  for (int i = 0; i < count; i++)
    array[i] = i;

  if (!in_order)
    random_shuffle(array, array + count);

  return array;
}

void printTimers() {
  
}

Timer* runTests(int thread_count, int loop_count, bool in_order) {
  LockFreeSkipList<int> skip_list;
  Tester tester(&skip_list);
  Timer* timer = new Timer();
  queue<pthread_t> threads;
  Callback<void>* cb_wrapper;
  int* values = createOperations(loop_count * thread_count, in_order);

  timer->start();

  Callback<void, int, int*, int>* cb = 
      makeCallableMany(&Tester::skipListTester, &tester);

  for (int i = 0; i < thread_count; i++) {
    Callback<void>* cb_wrapper = 
        makeCallableOnce(&Callback<void, int, int*, int>::operator(), 
                         cb,
                         i * loop_count,
                         values,
                         loop_count);

    threads.push(makeThread(cb_wrapper));
  }

  for (int i = 0; i < thread_count; i++) {
    pthread_join(threads.front(), NULL);
    threads.pop();
  }

  timer->end();

  return timer;
}

void testStarter(int threadOneCount, 
                 int threadTwoCount, 
                 int threadThreeCount,
                 int loop_count,
                 bool in_order) {

}



} // unamed namespace

int main(int argc, char* argv[]) {
  Timer* timer_one = NULL;  // timer results
  Timer* timer_two = NULL;

}