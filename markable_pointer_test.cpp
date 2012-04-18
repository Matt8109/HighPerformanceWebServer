#include "markable_pointer.hpp"
#include "test_unit.hpp"

namespace {

using lock_free::MarkablePointer;

//
// Test Cases
//

TEST(Basics, Initialization) {
  int i;
  int* pi = &i;
  EXPECT_FALSE(MarkablePointer<int>::isMarked(pi));
}

TEST(Basics, Mark) {
  int i;
  int* pi = &i;
  int* marked = MarkablePointer<int>::mark(pi);
  EXPECT_FALSE(MarkablePointer<int>::isMarked(pi));
  EXPECT_TRUE(MarkablePointer<int>::isMarked(marked));
}

TEST(Basics, Unmark) {
  int i;
  int* pi = &i;

  int* marked = MarkablePointer<int>::mark(pi);
  EXPECT_TRUE(MarkablePointer<int>::isMarked(marked));
  int* unmarked = MarkablePointer<int>::unmark(marked);
  EXPECT_FALSE(MarkablePointer<int>::isMarked(unmarked));
}

} // unnamed namespace

int main(int argc, char *argv[]) {
  return RUN_TESTS(argc, argv);
}

