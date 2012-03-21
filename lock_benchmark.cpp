#include <iostream>
#include <pthread.h>
#include <string>
#include <queue>

#include "lock.hpp"
#include "spinlock.hpp"
#include "test_util.hpp"
#include "thread.hpp"
#include "timer.hpp"

using base::Mutex;
using base::Spinlock;

namespace {

using base::Callback;
using base::makeCallableMany;
using base::makeThread;
using base::Timer;
using std::queue;
using std::string;
using test::Counter;

struct Tester {
  template<typename T>
  void BusyTestMethod(T lock, int loop_count, int work_count) {
    Counter counter;

    for (int i = 0; i < loop_count; i++) {
      lock->lock();

      for (int j = 0; j < work_count; j++)
        counter.inc();

      lock->unlock();
    }
  }
};

template<typename T>
Timer* LockTester(T lock, 
                 int thread_count, 
                 int loop_count, 
                 int work_count) {
  Tester tester;
  Timer* timer = new Timer;
  queue<pthread_t> threads;

  Callback<void, T, int, int, int>* cb = 
      makeCallableMany(&Tester::BusyTestMethod, 
                       tester,
                       lock,
                       thread_count,
                       loop_count,
                       work_count);

  timer->start();

  for (int i = 0; i < thread_count; i++) {
     threads.push(makeThread(cb));
  }

  for (int i = 0; i < thread_count; i++) {
    pthread_join(threads.front(), NULL);
    threads.pop();
  }

  timer->end();

  return timer;

  delete cb;
}

}

int main(int argc, char* argv[]) {
  std::cout << "Starting Small Method, 2 Threads";


}

void testStarter(int thread_count, int loop_count, int work_count) {
  Mutex* mutex = new Mutex();
  Spinlock* spinlock = new Spinlock();
  Timer* timer_one, timer_two;

  timer_one = LockTester<Mutex*>(mutex, thread_count, loop_count, work_count);


}

void printResults(string lock_one, 
          string lock_two,
          Timer* timer_one, 
          Timer* timer_two) {
  string winner;
  double total_time = timer_one->elapsed() + timer_two->elapsed();

  winner = timer_one->elapsed() > timer_two->elapsed() ? lock_one : lock_two;

  std::cout << winner << " was faster." << std::endl;
  
  // show the percentage of time taken out of the total
  std::cout << lock_one << " - " << timer_one->elapsed()
            << "(" << (int)(timer_one->elapsed() / total_time * 100) << ")"
            << std::endl;

  std::cout << lock_two << " - " << timer_two->elapsed()
            << "(" << (int)(timer_two->elapsed() / total_time * 100) << ")"
            << std::endl;
}