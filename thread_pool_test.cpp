#include "thread_pool_normal.hpp"
#include "test_unit.hpp"

namespace {

using base::ThreadPool;
using base::ThreadPoolNormal;

TEST(Group, Case) {
  EXPECT_TRUE(true);
}

} // unnammed namespace

int main(int argc, char* argv[]) {
  return RUN_TESTS(argc, argv);
}
