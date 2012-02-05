#include <iostream>
#include <sstream>

#include "callback.hpp"
#include "thread_pool_normal.hpp"
#include "thread_pool_fast.hpp"
#include "timer.hpp"

namespace {

using base::Callback;
using base::makeCallableMany;
using base::ThreadPoolNormal;
using base::ThreadPoolFast;
using base::Timer;

template<typename PoolType>
void FastConsumer() {
  std::cout << "Fast Consumer:\t";
  std::cout << std::endl;
}

template<typename PoolType>
void SlowConsumer() {
  std::cout << "Slow Consumer:\t";
  std::cout << std::endl;
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
