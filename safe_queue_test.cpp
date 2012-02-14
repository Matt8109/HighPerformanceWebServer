#include <stdlib.h>
#include <pthread.h>
#include "safe_queue.hpp"
#include "test_unit.hpp"

namespace {

using base::SafeQueue;

TEST(Simple, Insertion) {
	SafeQueue<int> queue;

	queue.push(5);

	EXPECT_EQ(queue.pop(), 5);
}

} // unnamed namespace

int main(int argc, char* argv[]) {
  return RUN_TESTS(argc, argv);
}
