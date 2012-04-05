#define HIT_COUNT_PER_THREAD 1000
#define MAX_THREADS 4

#include <iostream>
#include <pthread.h>

#include "callback.hpp"
#include "request_stats.hpp"
#include "server_stat_buffer.hpp"
#include "test_unit.hpp"
#include "thread.hpp"
#include "ticks_clock.hpp"

using base::makeCallableMany;
using base::RequestStats;
using base::ServerStatBuffer;
using base::TicksClock;

namespace {

struct Tester {
  void StatTester(int thread_num, RequestStats* stats) {
    for (int i =0; i < HIT_COUNT_PER_THREAD; i++)
      stats->finishedRequest(thread_num, TicksClock::getTicks());
  }
};

TEST(BufferTest, TestZeroHits) {
  ServerStatBuffer buf(10);

  EXPECT_EQ(buf.getHits(), 0);
}

TEST(BufferTest, FillTest) {
  ServerStatBuffer buf(50);

  for (int i = 0; i < 1000; i++)
    buf.hit();

  EXPECT_EQ(1000, buf.getHits());
}

}  // unamed namespace

int main(int argc, char *argv[]) {

  return RUN_TESTS(argc, argv);
}
