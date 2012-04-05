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
