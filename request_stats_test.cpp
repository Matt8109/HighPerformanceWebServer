#include <iostream>
#include <pthread.h>

#include "request_stats.hpp"
#include "server_stat_buffer.hpp"
#include "test_unit.hpp"
#include "ticks_clock.hpp"

using base::ServerStatBuffer;
using base::TicksClock;

namespace {

class Tester {
public:
  Tester() { }
  ~Tester() { }

private:
};

TEST(BufferTest, TestZeroHits) {
  ServerStatBuffer buf(100);

  std::cout << TicksClock::ticksPerSecond();

  EXPECT_EQ(buf.getHits(), 0);
}

TEST(BufferTest, FillTest) {
  ServerStatBuffer buf(1000);

  for (int i = 0; i < TicksClock::ticksPerSecond() * 2; i++)
    buf.hit();

  EXPECT_NEQ(buf.getHits(), 0);
  EXPECT_GT(TicksClock::ticksPerSecond(), buf.getHits()); // hits < clock 
}

}  // unamed namespace

int main(int argc, char *argv[]) {

  return RUN_TESTS(argc, argv);
}
