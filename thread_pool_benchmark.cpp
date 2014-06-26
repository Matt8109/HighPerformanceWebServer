// Code copyright Alberto Lerner and Matthew Mancuso
// See git blame for details

#define THREAD_COUNT_SM 2 //small test size
#define THREAD_COUNT_MD 8 // medium test size
#define THREAD_COUNT_LG 20 // large test size
#define LOOP_COUNT 500000 // the number of tasks for each thread_pool
#define TASK_SLOW_COUNT 500 // time to waste in the slow consumer per action
#define TASK_ADD_SLOW_COUNT 50 // time to waste before queuing new tasks

#include <iostream>
#include <sstream>

#include "callback.hpp"
#include "thread_pool_normal.hpp"
#include "thread_pool_fast.hpp"
#include "test_util.hpp"
#include "timer.hpp"

namespace {

using base::Callback;
using base::makeCallableMany;
using base::ThreadPoolNormal;
using base::ThreadPoolFast;
using base::Timer;
using test::TestClass;

// Seem to be needed to be called from within the namespace
template<typename PoolType>
void FastTestLogic(int pool_size);

template<typename PoolType>
void SlowTestLogic(int pool_size); 

template<typename PoolType>
void FastConsumer() {
  std::cout << "\nFast Consumer:\t";
  std::cout << std::endl << "\t";

  FastTestLogic<PoolType>(THREAD_COUNT_SM);
  std::cout << " | ";
  
  FastTestLogic<PoolType>(THREAD_COUNT_MD);
  std::cout << " | ";

  FastTestLogic<PoolType>(THREAD_COUNT_LG);

}

template<typename PoolType>
void FastTestLogic(int pool_size) {
  // Essentially this method will add a bunch of fast tasks to the
  // thread pool, and will do busywork every so often based on the 
  // number of threads in the pool to try to keep worker threads available
  bool flip = false;
  Timer timer;

  PoolType* thread_pool = new PoolType(pool_size);

  TestClass thread_test(thread_pool);

  Callback<void>* increase_task = 
    makeCallableMany(&TestClass::increase, &thread_test);

  Callback<void>* hit_task =
    makeCallableMany(&TestClass::hit, &thread_test);

  timer.start();

  for (int i = 0; i < LOOP_COUNT; i++) {
    if (i % pool_size == 0) { 
      // slow down adding tasks every so often, keep pool "mostly" non full
      for (int i = 0; i < TASK_ADD_SLOW_COUNT; i++)
        flip=!flip;
    }

    thread_pool->addTask(increase_task);

    if (i % 10 == 0) // Every so often throw in a different task
      thread_pool->addTask(hit_task);
  }

  thread_pool->stop();

  timer.end();

  std::cout << timer.elapsed() << " with " << pool_size << " threads";

  delete thread_pool;
  delete increase_task;
  delete hit_task;
}

template<typename PoolType>
void SlowConsumer() {
  std::cout << "\nSlow Consumer:\t";
  std::cout << std::endl << "\t";

  SlowTestLogic<PoolType>(THREAD_COUNT_SM);
  std::cout << " | ";

  SlowTestLogic<PoolType>(THREAD_COUNT_MD);
  std::cout << " | ";

  SlowTestLogic<PoolType>(THREAD_COUNT_LG);
}

template<typename PoolType>
void SlowTestLogic(int pool_size) {
  // A slower benchmark test. The test driver never pauses while adding threads
  // and the threads methods themselves have a built in slowdown that will cause
  // a backlog of tasks to form in the thread pool.
  Timer timer;
  PoolType* thread_pool = new PoolType(pool_size);

  TestClass thread_test(thread_pool);

  Callback<void, int>* internal_cb = 
    makeCallableMany(&TestClass::slowIncrease, &thread_test);

  Callback<void>* wrapper_cb = 
    makeCallableMany(
        &Callback<void, int>::operator(), internal_cb, TASK_SLOW_COUNT);

  timer.start();

  for (int i = 0; i<LOOP_COUNT; i++)
    thread_pool->addTask(wrapper_cb);

  thread_pool->stop();

  timer.end();

  std::cout << timer.elapsed() << " with " << pool_size << " threads";

  delete thread_pool;
  delete wrapper_cb;
  delete internal_cb;
}

}  // unnamed namespace

void usage(int argc, char* argv[]) {
  std::cout << "Usage: " << argv[0] << " [1 | 2]" << std::endl;
  std::cout << "  1 is normal thread pool" << std::endl;
  std::cout << "  2 is fast thread pool" << std::endl;
  std::cout << "  defaul is to run both " << std::endl;
}

int main(int argc, char* argv[]) {
  bool all = false;
  bool num[4] = {false};
  if (argc == 1) {
    all = true;
  } else if (argc == 2) {
    std::istringstream is(argv[1]);
    int i = 0;
    is >> i;
    if (i>0 && i<=4) {
      num[i-1] = true;
    } else {
      usage(argc, argv);
      return -1;
    }
  } else {
    usage(argc, argv);
    return -1;
  }
  
  std::cout << "\nThese take some time, please give them a moment.\n" 
            << "Even if the cursor stops blinking.\n\n"
            << std::flush;

  // no queueing of tasks really
  if (all || num[0]) {
    FastConsumer<ThreadPoolNormal>();
  }
  if (all || num[1]) {
    FastConsumer<ThreadPoolFast>();
  }

  // force queue building
  if (all || num[0]) {
    SlowConsumer<ThreadPoolNormal>();
  }
  if (all || num[1]) {
    SlowConsumer<ThreadPoolFast>();
  }

  std::cout << "\n\n"; // Just to make the return to the cmd line nicer

  return 0;
}
