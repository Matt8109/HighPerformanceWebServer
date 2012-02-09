#define CORE_COUNT 8 //the number of cores you have/threads to create
#define LOOP_COUNT 5000 // the number of tasks for each thread_pool
#define SLOW_LOOP_COUNT 500 // time to waste in the slow consumer per action

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
using test::TestThread;

template<typename PoolType>
void FastConsumer() {
  bool flip = false;
	Timer timer;

	std::cout << "Fast Consumer:\t";
  std::cout << std::endl;

	PoolType* thread_pool = new PoolType(CORE_COUNT);

	TestThread thread_test(thread_pool);

	Callback<void>* callback = 
		makeCallableMany(&TestThread::increase, &thread_test);

	timer.start();

	for (int i = 0; i<LOOP_COUNT; i++) {
		if (i % (CORE_COUNT+2) == 0) { 
			// slow down adding tasks every so often, keep pool "mostly" non full
			flip=!flip;
		}

		thread_pool->addTask(callback);
	}

	thread_pool->stop();

	timer.end();

	std::cout << "\t" << timer.elapsed() << std::endl;

	delete thread_pool;
	delete callback;
}

template<typename PoolType>
void SlowConsumer() {
	Timer timer;

  std::cout << "Slow Consumer:\t";
  std::cout << std::endl;

	PoolType* thread_pool = new PoolType(CORE_COUNT);

	TestThread thread_test(thread_pool);

	Callback<void, int>* internal_cb = 
		makeCallableMany(&TestThread::slowIncrease, &thread_test);

	Callback<void>* wrapper_cb = 
		makeCallableMany(
				&Callback<void, int>::operator(), internal_cb, SLOW_LOOP_COUNT);

	timer.start();

	for (int i = 0; i<LOOP_COUNT; i++)
		thread_pool->addTask(wrapper_cb);

	thread_pool->stop();

	timer.end();

	std::cout << "\t" << timer.elapsed() << std::endl;

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

  return 0;
}
