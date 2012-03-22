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
using base::Timer;

namespace {

using base::Callback;
using base::makeCallableMany;
using base::makeThread;
using base::Timer;
using std::queue;
using std::string;
using test::Counter;
using base::Mutex;
using base::Spinlock;

struct Tester {
  template<typename T>
  void busyTestMethod(T lock, int loop_count, int work_count) {
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
  Timer* timer = new Timer();
  queue<pthread_t> threads;

  tester.busyTestMethod<T>( lock,
                       loop_count,
                       work_count);

  Callback<void>* cb = 
      makeCallableMany(&Tester::busyTestMethod<T>, 
                       &tester,
                       lock,
                       loop_count,
                       work_count);

  Callback<void>* cb_wrapper = makeCallableMany(&Callback<void>::operator(), cb);

  timer->start();

  for (int i = 0; i < thread_count; i++) {
     threads.push(makeThread(cb_wrapper));
  }

  for (int i = 0; i < thread_count; i++) {
    pthread_join(threads.front(), NULL);
    threads.pop();
  }

  timer->end();

  //delete cb_wrapper;
  //delete cb;

  return timer;
}

void printResults(string lock_one, 
                  string lock_two,
                  Timer* timer_one, 
                  Timer* timer_two) {
  string winner;
  double total_time = timer_one->elapsed() + timer_two->elapsed();

  winner = timer_one->elapsed() < timer_two->elapsed() ? lock_one : lock_two;

  std::cout << winner << " was faster." << std::endl;
  
  // show the percentage of time taken out of the total
  std::cout << lock_one << " - " << timer_one->elapsed()
            << "( " << (int)(timer_one->elapsed() / total_time * 100) << "% )"
            << std::endl;

  std::cout << lock_two << " - " << timer_two->elapsed()
            << "( " << (int)(timer_two->elapsed() / total_time * 100) << "% )"
            << std::endl;
}

void testStarter(int thread_count, int loop_count, int work_count) {
  Mutex* mutex = new Mutex();
  Spinlock* spinlock = new Spinlock();
  Timer* timer_one;
  Timer* timer_two;

  timer_one = LockTester<Mutex*>(mutex, thread_count, loop_count, work_count);
  timer_two = LockTester<Spinlock*>(spinlock, 
                                    thread_count, 
                                    loop_count, 
                                    work_count);

  printResults("Mutex", "Spinlock", timer_one, timer_two);

  delete timer_one;
  delete timer_two;
  delete mutex;
  delete spinlock;
}

}

int main(int argc, char* argv[]) {
  std::cout << "2 Threads, 200k iterations, Small Method" << std::endl;
  testStarter(2, 200000, 2);

  std::cout << std::endl << std::endl;
  std::cout << "4 Threads, 200k iterations, Small Method" << std::endl;
  testStarter(4, 200000, 2);

  std::cout << std::endl << std::endl;
  std::cout << "8 Threads, 200k iterations, Small Method" << std::endl;
  testStarter(8, 200000, 2);

  std::cout << std::endl << std::endl;
  std::cout << "2 Threads, 200k iterations, Medium Method" << std::endl;
  testStarter(2, 200000, 200);

  std::cout << std::endl << std::endl;
  std::cout << "4 Threads, 200k iterations, Medium Method" << std::endl;
  testStarter(4, 200000, 200);

  std::cout << std::endl << std::endl;
  std::cout << "8 Threads, 200k iterations, Medium Method" << std::endl;
  testStarter(8, 200000, 200);

  std::cout << std::endl << std::endl;
  std::cout << "32 Threads, 2k iterations, Large Method" << std::endl;
  testStarter(32, 2000, 500);

  std::cout << std::endl << std::endl;
  std::cout << "64 Threads, 2k iterations, Large Method" << std::endl;
  testStarter(64, 2000, 500);

  std::cout << std::endl << std::endl;
  std::cout << "128 Threads, 2k iterations, Large Method" << std::endl;
  testStarter(128, 2000, 500);

  std::cout << std::endl << std::endl;
  std::cout << "In cases with a light to medium method size and thread \n"
            << "the spinlock is best. However when the number of threads\n"
            << "grows, along with the method time, blocking can be better\n"
            << "so as not to sap the CPU spinning when a real thread can run."
            << std::endl << std::endl;
}