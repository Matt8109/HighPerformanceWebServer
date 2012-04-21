#include "hazard_pointers.hpp"
#include "test_unit.hpp"

namespace {

using lock_free::HazardPointers;

TEST(Basics, EachThreadItsOwnHazards) {
  HazardPointers<int, 1 /* hazard pointer */> hps(2 /* two threads */);
  const int THREAD_ZERO = 0;
  const int THREAD_ONE = 1;

  int** HPA = hps.getHPRec(THREAD_ZERO);
  int** HPB = hps.getHPRec(THREAD_ONE);
  EXPECT_NEQ(HPA, reinterpret_cast<int**>(NULL));
  EXPECT_NEQ(HPB, reinterpret_cast<int**>(NULL));
  EXPECT_NEQ(HPA, HPB);
}

TEST(Basics, ReclamationThreshold) {
  HazardPointers<int, 1 /* hazard pointer */> hps(2 /* two threads */);
  const int THREAD_ZERO = 0;

  int* pi = new int;
  int num_retired = hps.retireNode(THREAD_ZERO, pi);  // ownership xfer
  EXPECT_EQ(num_retired, 0);

  // Call internal reclamation, freeing nodes if possible.
  EXPECT_EQ(hps.maybeFreeNodes(THREAD_ZERO), 1);
}

TEST(Basics, ActiveHazardPreventsReclamation) {
  HazardPointers<int, 1 /* hazard pointer */> hps(2 /* two threads */);
  const int THREAD_ZERO = 0;
  const int THREAD_ONE = 1;

  // Thread 1 marks pi as a hazard.
  int* pi = new int;
  int** HPB = hps.getHPRec(THREAD_ONE);
  HPB[0] = pi;

  // Thread 0 retires pi. Even though thread 0's retired list is
  // beyond threshold, it can only reclaim pi after thread one stops
  // using it. Make sure to lower the reclamation threshold
  // artificially to zero.
  hps.MAX_RETIRED_NODES_PER_THREAD = 0;
  int num_retired = hps.retireNode(THREAD_ZERO, pi);  // ownwership xfer
  EXPECT_EQ(num_retired, 0);

  // Have thread 1 stop mentioning 'pi' as a hazard and call the
  // internal reclamation routine.
  HPB[0] = NULL;
  EXPECT_EQ(hps.maybeFreeNodes(THREAD_ZERO), 1);
}

}  // unnamed namespace

int main(int argc, char* argv[]) {
  return RUN_TESTS(argc, argv);
}
