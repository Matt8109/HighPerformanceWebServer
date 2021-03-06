// Code copyright Alberto Lerner and Matthew Mancuso
// See git blame for details

#define HIT_COUNT 2000
#define THREADS 4

#include <iostream>
#include <pthread.h>

#include "callback.hpp"
#include "request_stats.hpp"
#include "server_stat_buffer.hpp"
#include "test_unit.hpp"
#include "thread.hpp"
#include "ticks_clock.hpp"

using base::Callback;
using base::makeCallableMany;
using base::makeCallableOnce;
using base::makeThread;
using base::RequestStats;
using base::ServerStatBuffer;
using base::TicksClock;

namespace {

struct Tester {
  void StatTester(int thread_num, RequestStats* stats) {
    for (int i = 0; i < HIT_COUNT; i++)
      stats->finishedRequest(thread_num, TicksClock::getTicks());
  }
};

TEST(BufferTest, TestZeroHits) {
  ServerStatBuffer buf(10);
  uint64_t tps = static_cast<uint64_t>(TicksClock::ticksPerSecond());

  EXPECT_EQ(buf.getHits(TicksClock::getTicks() % tps), 0);
}

TEST(BufferTest, FillTest) {
  ServerStatBuffer buf(10);
  uint64_t tps = static_cast<uint64_t>(TicksClock::ticksPerSecond());

  for (int i = 0; i < 1000; i++)
    buf.hit(TicksClock::getTicks() % tps);

  EXPECT_EQ(1000, buf.getHits(tps));
}

TEST(RequestStats, RollOverTestFourSeconds) {
  int start = TicksClock::getTicks() / TicksClock::ticksPerSecond();
  RequestStats stats(1);
  uint32_t result = 0;

  // run for around three seconds so we loop around the buffer
  while (TicksClock::getTicks() / TicksClock::ticksPerSecond() - start < 4) {
    stats.finishedRequest(0, TicksClock::getTicks());
  }

  stats.getStats(TicksClock::getTicks(), &result);

  EXPECT_GT(TicksClock::ticksPerSecond(), result)
}

TEST(RequestStats, MultilpleThreadAndSeconds) {
  pthread_t threads[THREADS];
  RequestStats stats(THREADS);
  ServerStatBuffer buf(50);
  Tester tester;
  uint32_t result = 0;

  for (int i = 0; i < THREADS; i++) {
    Callback<void, int, RequestStats*>* cb = 
        makeCallableOnce(&Tester::StatTester, &tester);

    Callback<void>* cb_wrapper = 
        makeCallableOnce(&Callback<void, int, RequestStats*>::operator(), 
                         cb,
                         i, 
                         &stats);

    threads[i] = makeThread(cb_wrapper);
  }

  for (int i = 0; i < THREADS; i++) 
    pthread_join(threads[i], NULL);

  stats.getStats(TicksClock::getTicks(), &result);

  // expect within 1% of expected
  EXPECT_GT(result, HIT_COUNT * THREADS * 0.99);
  EXPECT_GT(HIT_COUNT * THREADS * 1.01, result);
}

}  // unamed namespace

int main(int argc, char *argv[]) {

  return RUN_TESTS(argc, argv);
}
