#include <pthread.h>
#include <iostream>

#include "ticks_clock.hpp"
#include "test_unit.hpp"
#include "request_stats.hpp"

using base::TicksClock;

namespace {

class Tester {
public:
	Tester() { }
	~Tester() { }

private:
};

TEST(Group, Case) {
  EXPECT_TRUE(true);
}

}  // unamed namespace

int main(int argc, char *argv[]) {

  return RUN_TESTS(argc, argv);
}
