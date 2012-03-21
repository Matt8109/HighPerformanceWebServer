#define CORE_COUNT 4
#define LOOP_COUNT 1000

#include <iostream>
#include <pthread.h>
#include <string>

#include "lock.hpp"
#include "spinlock.hpp"
#include "test_util.hpp"
#include "thread.hpp"
#include "timer.hpp"

namespace {

using base::Callback;
using base::makeCallableMany;
using base::makeThread;
using base::Timer;
using std::string;
using test::Counter;

struct Tester {
  template<typename T>
  void BusyTestMethod(T* lock, int work_count) {
    Counter counter;

    for (int i = 0; i < LOOP_COUNT; i++) {
      lock->lock();

      for (int j = 0; j < work_count; j++)
        counter.inc();

      lock->unlock();
    }
  }
};

template<typename T>
Timer* MaxCoreTestFast(T lock) {
  Tester tester;
  Timer* timer = new Timer;

  pthread_t threads[CORE_COUNT];


  Callback<void, T*, int>* cb = makeCallableMany(&Tester::BusyTestMethod, 
                           &tester,
                           &lock,
                           5);

  timer->start();

  for (int i = 0; i < CORE_COUNT; i++) {
    threads[i] = makeThread(cb);
  }

  for (int i = 0; i < CORE_COUNT; i++) {
    pthread_join(threads[i], NULL);
  }

  timer->end();

  return timer;

  delete cb;
}

}

int main(int argc, char* argv[]) {

}

void PrintResults(string lock_one, 
          string lock_two,
          Timer* timer_one, 
          Timer* timer_two) {
  string winner;
  double total_time = timer_one->elapsed() + timer_two->elapsed();

  winner = timer_one->elapsed() > timer_two->elapsed() ? lock_one : lock_two;

  std::cout << winner << " was faster." << endl;
  std::cout << lock_one << " - " << timer_one->elapsed
            << "(" << (int)(timer_one->elapsed() / total_time * 100) << ")";

}