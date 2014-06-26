// Code copyright Alberto Lerner and Matthew Mancuso
// See git blame for details

#include <pthread.h>  // barriers

#include "callback.hpp"
#include "hazard_pointers.hpp"
#include "lock_free_list.hpp"
#include "op_generator.hpp"
#include "signal_handler.hpp"
#include "test_unit.hpp"
#include "thread_pool_fast.hpp"

namespace {

using base::Callback;
using base::makeCallableOnce;
using base::ThreadPoolFast;
using lock_free::HazardPointers;
using lock_free::LockFreeList;
using lock_free::OpGenerator;

// A test helper that can issue operations against a shared list from
// multiple threads. Each thread operates on distinct set of
// operations. See 'op generators' to define that set.
//
// All threads start executing together and an external observer is
// synchronized when all the thread finish running.
//
// Notes:
//
//   + synchronizing the external observer is *not* optional. It
//     participates on a barrier along with all the worker threads.
//
class Tester {
public:
  Tester(LockFreeList<int>* list, int num_ops, int num_threads)
    : list_(list),
      num_ops_(num_ops),
      num_threads_(num_threads),
      in_error_(false) {
    pthread_barrier_init(&beg_barrier_, NULL, num_threads_);
    pthread_barrier_init(&end_barrier_, NULL, num_threads_+1);
  }

  ~Tester() {
    pthread_barrier_destroy(&beg_barrier_);
    pthread_barrier_destroy(&end_barrier_);
  }

  void runWorker(int thread_num, /* const */ OpGenerator* op_gen) {
    int* op_array;
    op_gen->genOps(num_threads_, thread_num, &op_array, num_ops_);
    pthread_barrier_wait(&beg_barrier_);
    applyOps(op_array, num_ops_);
    delete [] op_array;
    pthread_barrier_wait(&end_barrier_);
  }

  // Must be called from the N+1'th thread (outside the pool).
  void waitWorkers() {
    pthread_barrier_wait(&end_barrier_);
  }

  bool ok() { return ! in_error_; }

private:
  LockFreeList<int>* list_;
  int num_ops_;
  int num_threads_;
  bool in_error_;

  pthread_barrier_t beg_barrier_;
  pthread_barrier_t end_barrier_;

  bool applyOps(int* op_array, size_t size) {
    for (size_t i= 0; i<size; i++) {
      const int  op = op_array[i];
      bool ok;
      if (op > 0) {
        ok = list_->insert(op);
      } else {
        ok = list_->remove(-op);
      }
      if (!ok) {
        std::cout << "    failed operation: " << op << std::endl;
        in_error_ = true;
        return false;
      }
    }
    return true;
  }
};

// Generates 'size' operations for each of the 'num_worker' callers,
// one at a time ('me'). Transfers ownership of the result, '*ops', to
// the caller.
//
// The operations consist of ascending inserts, at the end of the
// list, and then ascending deletions, from the beginning of the
// list. Each 'me' caller operates on a non-overlapping set of
// operations.
//
class GenNonOverlappingInsertsDeletes : public OpGenerator {
  void genOps(int num_workers, int me, int** ops, int size) {
    *ops = new int[size];
    int* firstHalf = *ops;
    int* secondHalf = &(*ops)[size/2];
    OpGenerator::AscPositiveStripe(num_workers, me, firstHalf, size/2);
    OpGenerator::DescNegativeStripe(num_workers, me, secondHalf, size/2);
  }
};

// Similar to GenNonOverlappingInsertsDeletes but insertions and
// deletions operations touch random portion of the list.
class GenNonOverlappingRandomOps : public OpGenerator {
  void genOps(int num_workers, int me, int** ops, int size) {
    *ops = new int[size];
    int* firstHalf = *ops;
    int* secondHalf = &(*ops)[size/2];
    OpGenerator::ShufflePositiveStripe(num_workers, me, firstHalf, size/2);
    OpGenerator::ShuffleNegativeStripe(num_workers, me, secondHalf, size/2);
  }
};

//
// Test Cases
//

TEST(Sequential, SimpleInsertion) {
  LockFreeList<char> l(1 /* single worker */);
  ThreadPoolFast::setMEForTest(0);

  // on empty list
  EXPECT_FALSE(l.lookup('a'));
  EXPECT_TRUE(l.insert('a'));

  // on non-empty list
  EXPECT_TRUE(l.lookup('a'));
  EXPECT_FALSE(l.lookup('b'));
}

TEST(Sequential, DuplicateInsertion) {
  LockFreeList<char> l(1 /* single worker */);
  ThreadPoolFast::setMEForTest(0);

  EXPECT_TRUE(l.insert('a'));
  EXPECT_FALSE(l.insert('a'));
  EXPECT_TRUE(l.lookup('a'));
}

TEST(Sequential, SimpleDeletion) {
  LockFreeList<char> l(1 /* single worker */);
  ThreadPoolFast::setMEForTest(0);

  // on empty list
  EXPECT_FALSE(l.remove('a'));

  // on non-empty list
  EXPECT_TRUE(l.insert('a'));
  EXPECT_TRUE(l.insert('b'));
  EXPECT_TRUE(l.remove('a'));
  EXPECT_FALSE(l.lookup('a'));
  EXPECT_TRUE(l.lookup('b'));

  // on newly empty list
  EXPECT_TRUE(l.remove('b'));
  EXPECT_FALSE(l.lookup('a'));
  EXPECT_FALSE(l.lookup('b'));
}

TEST(Sequential, FailedDeletion) {
  LockFreeList<char> l(1 /* single worker */);
  ThreadPoolFast::setMEForTest(0);

  EXPECT_TRUE(l.insert('a'));
  EXPECT_FALSE(l.remove('b'));
}

TEST(Sequential, Reclaiming) {
  LockFreeList<int> l(1 /* single worker */);
  ThreadPoolFast::setMEForTest(0);

  int reclaim_threshold = HazardPointers<int,2>::MAX_RETIRED_NODES_PER_THREAD;
  EXPECT_TRUE(l.insert(0));
  for (int i=1; i<2*reclaim_threshold; i++) {
    EXPECT_TRUE(l.lookup(i-1));
    EXPECT_TRUE(l.insert(i));
    EXPECT_TRUE(l.remove(i-1));
  }
  EXPECT_TRUE(l.remove(2*reclaim_threshold-1));
}

TEST(Concurrency, InsertionThenDeletion) {
  const int NUM_THREADS = 16;
  const int NUM_OPS = 1000; // # of ins/dels done by each thread

  LockFreeList<int> l(NUM_THREADS);
  ThreadPoolFast pool(NUM_THREADS);
  Tester tester(&l, NUM_OPS, NUM_THREADS);
  OpGenerator* genops = new GenNonOverlappingInsertsDeletes;

  for (int i=0; i<NUM_THREADS; i++) {
    Callback<void>* cb = makeCallableOnce(&Tester::runWorker,
                                          &tester,
                                          i,
                                          genops);
    pool.addTask(cb);
  }
  tester.waitWorkers();
  pool.stop();

  EXPECT_TRUE(tester.ok());
  EXPECT_TRUE(l.checkIntegrity());

  delete genops;
}

TEST(Concurrency, RoundsOfRandomOps) {
  const int NUM_THREADS = 16;
  const int NUM_OPS = 1000;
  const int NUM_ROUNDS = 10;

  LockFreeList<int> l(NUM_THREADS);
  ThreadPoolFast pool(NUM_THREADS);
  Tester tester(&l, NUM_OPS, NUM_THREADS);
  OpGenerator* genops = new GenNonOverlappingRandomOps;

  for (int i=0; i<NUM_ROUNDS; i++) {
    for (int j=0; j<NUM_THREADS; j++) {
      Callback<void>* cb = makeCallableOnce(&Tester::runWorker,
                                            &tester,
                                            j,
                                            genops);
      pool.addTask(cb);
    }
    tester.waitWorkers();

    EXPECT_TRUE(tester.ok());
    EXPECT_TRUE(l.checkIntegrity());
  }

  pool.stop();
  delete genops;
}

} // unnamed namespace

int main(int argc, char *argv[]) {
  base::installSignalHandler();
  return RUN_TESTS(argc,argv);
}
