// Code copyright Alberto Lerner and Matthew Mancuso
// See git blame for details

#include <iostream>
#include <math.h>
#include <pthread.h>
#include <queue>
#include <string>

#include "lock.hpp"
#include "spinlock.hpp"
#include "spinlock_mcs.hpp"
#include "test_util.hpp"
#include "thread.hpp"
#include "timer.hpp"

using base::Callback;
using base::makeCallableMany;
using base::makeThread;
using base::Mutex;
using base::Spinlock;
using base::SpinlockMcs;
using base::Timer;
using std::queue;
using std::string;
using test::Counter;

namespace {

const int CORE_COUNT = sysconf( _SC_NPROCESSORS_ONLN );

struct Tester {
public:
  queue<double> thread_timer_results;

  template<typename T>
  void busyTestMethod(T lock, int loop_count, int work_count) {
    queue<double> timers;   // keep track of all lock acquisition times
    Timer timer;            // the current timer
    Counter counter;

    for (int i = 0; i < loop_count; i++) {
      timer.reset();
      timer.start();

      lock->lock();

      timer.end();

      for (int j = 0; j < work_count; j++)
        counter.inc();

      lock->unlock();

      timers.push(timer.elapsed());
    }

    // copy our results to the main list of results
    results_lock_.lock();
    for (unsigned int i = 0; i < timers.size(); i++) {
      thread_timer_results.push(timers.front());
      timers.pop();
    }

    results_lock_.unlock();
  }

private:
  Mutex results_lock_;
};

double calculate_std(queue<double> values) {
  double average = 0.0;
  double deviation_sum = 0.0;

  for (unsigned int i = 0; i < values.size(); i++) {    // calculate average
    average = average + values.front();
    values.push(values.front());
    values.pop();
  }

  average = average / values.size();

  for (unsigned int i = 0; i < values.size(); i++) {    // calculate deviations
    values.push(values.front() - average);
    values.pop();
  }

  for (unsigned int i = 0; i < values.size(); i++) {
    int square = values.front() * values.front();

    values.pop();
    values.push(square);
  }

  for (unsigned int i = 0; i < values.size(); i++) {
    deviation_sum += values.front();
    values.pop();
  }

  deviation_sum = deviation_sum / (values.size() - 1);

  return sqrt(deviation_sum);
}

template<typename T>
Timer* LockTester(T lock, 
                 int thread_count, 
                 int loop_count, 
                 int work_count,
                 double* std_dev) {
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

  Callback<void>* cb_wrapper = 
      makeCallableMany(&Callback<void>::operator(), cb);

  timer->start();

  for (int i = 0; i < thread_count; i++) {
     threads.push(makeThread(cb_wrapper));
  }

  for (int i = 0; i < thread_count; i++) {
    pthread_join(threads.front(), NULL);
    threads.pop();
  }

  timer->end();

  delete cb_wrapper;
  delete cb;

  std_dev = new double(calculate_std(tester.thread_timer_results));
  std_dev = new double(5.5);

  return timer;
}

void printResults(string lock_one, 
                  string lock_two,
                  string lock_three,
                  Timer* timer_one, 
                  Timer* timer_two,
                  Timer* timer_three,
                  Timer* timer_one_results,
                  Timer* timer_two_results,
                  Timer* timer_three_results,
                  double* std_one,
                  double* std_two,
                  double* std_three,
                  int thread_count) {
  double total_time = timer_one->elapsed() 
                      + timer_two->elapsed()
                      + timer_three->elapsed();

  std::cout << std::endl << "\t" << thread_count << " thread(s)" << std::endl;
  std::cout << "\t\t" << lock_one;
  
  std::cout << " - " << timer_one->elapsed();

  if (timer_one_results) 
    std::cout << "(" << timer_one->elapsed() - timer_two_results->elapsed() 
         << ")";
  else
    std::cout << "\t";

  std::cout << "\t";

  for (int i = 0; i < (int)(timer_one->elapsed() / total_time * 20); i++ )
    std::cout << "|";

  std::cout << std::endl;
  std::cout << "\t\t" << lock_two;
  
  std::cout << " - " << timer_two->elapsed();

  if (timer_two_results)
    std::cout << "(" << timer_two->elapsed() - timer_two_results->elapsed() 
         << ")";
  else
    std::cout << "\t";

  std::cout << "\t";

  for (int i = 0; i < (int)(timer_two->elapsed() / total_time * 20); i++ )
    std::cout << "|";

  std::cout << std::endl;
  std::cout << "\t\t" << lock_three;
  
  std::cout << " - " << timer_three->elapsed();

  if (timer_three_results)
    std::cout << "(" << timer_three->elapsed() - timer_three_results->elapsed() 
         << ")";
  else
    std::cout << "\t";

  std::cout << "\t";

  for (int i = 0; i < (int)(timer_three->elapsed() / total_time * 20); i++ )
    std::cout << "|";

  std::cout << std::endl << std::endl << "Lock Acquire Standard Deviation:"
            << std::endl << "\t" << lock_one << " - " 
            << (std_one == NULL ? 0.0 : *std_one)
            << std::endl << "\t" << lock_two << " - "
            << (std_two == NULL ? 0.0 : *std_two)
            << std::endl << "\t" << lock_two << " - "
            << (std_three == NULL ? 0.0 : *std_three)
            << std::endl << std::endl;
}

void testStarter(int thread_count, 
                 int loop_count,
                 int work_count, 
                 Timer** timer_one_results,
                 Timer** timer_two_results,
                 Timer** timer_three_results) {
  Mutex* mutex = new Mutex();
  Spinlock* spinlock = new Spinlock();
  SpinlockMcs* spinlock_mcs = new SpinlockMcs();
  Timer* timer_one;
  Timer* timer_two;
  Timer* timer_three;
  double* std_dev_one = NULL;
  double* std_dev_two = NULL;
  double* std_dev_three = NULL;

  timer_one = LockTester<Mutex*>(mutex, 
                                 thread_count, 
                                 loop_count, 
                                 work_count,
                                 std_dev_one);

  timer_two = LockTester<Spinlock*>(spinlock, 
                                    thread_count, 
                                    loop_count, 
                                    work_count,
                                    std_dev_two);

  timer_three = LockTester<SpinlockMcs*>(spinlock_mcs, 
                                    thread_count, 
                                    loop_count, 
                                    work_count,
                                    std_dev_three);


  printResults("Mutex   ",
               "Spinlock",
               "SpinlockMcs",
               timer_one, 
               timer_two,
               timer_three,
               *timer_one_results,
               *timer_two_results,
               *timer_three_results,
               std_dev_one,
               std_dev_two,
               std_dev_three, 
               thread_count);

  if (*timer_one_results)      // delete old results before we swap them out
    delete *timer_one_results;
  if (*timer_two_results)
    delete *timer_two_results;
  if (*timer_three_results)
    delete *timer_three_results;

  *timer_one_results = timer_one;
  *timer_two_results = timer_two;
  *timer_three_results = timer_three;
  delete mutex;
  delete spinlock;
  delete spinlock_mcs;
}

}

int main(int argc, char* argv[]) {
  Timer* timer_one = NULL;  // timer results
  Timer* timer_two = NULL;
  Timer* timer_three = NULL;
  std::cout << "200k iterations, Medium Critical Section" << std::endl;

  std::cout << std::endl;
  testStarter(CORE_COUNT, 200000, 500, &timer_one, &timer_two, &timer_three);

  delete timer_one;
  delete timer_two;

  timer_one = NULL;
  timer_two = NULL;

  std::cout << "Formatting:\n"
            << "Type (* = winner) - Time taken (Change in time from last)\n"
            << "Pipes = percentage of time taken between the two tests.\n\n";
}