#ifndef MCP_LOCK_FREE_OP_GENERATOR_HEADER
#define MCP_LOCK_FREE_OP_GENERATOR_HEADER

namespace lock_free {

// An interface for generators of series of integers. A series can be
// used by a test to determine which operations would be applied
// against a test structure. For example, a series <1, 2, 3, -1, -2,
// 3> could insert 1, then 2, then 3 on a list and then delete them in
// that same order.
//
// A generator also takes the number of workers that will operate
// together along with for which worker is a given call being made.
// The idea here is that we can partition the operation space in ways
// so to allow a multi-threaded test against a data structure.  For
// example, we can have two worker's series <1, 3, 5, -1, -3, -5> and
// <2, 4, 6, -2, -4, -6>, meaning the operations are 'stripped' among
// the workers that can potentially be working in parallel.
//
// There are a few basic generators available that can be used to
// compose more sophisticated ones.
//
struct OpGenerator {
  // Fills '*ops' with the operations for 'me' of a total of
  // 'num_workers' workers.  Transfers ownwership of 'ops', which has
  // size 'size', to the caller.
  virtual void genOps(int num_workers, int me, int** ops, int size) = 0;

  // A collection of basic striping methods (each of the workers operate
  // on an exclusive set of positions).

  // num workers: k
  // me:0, 1, k+1, 2k+1,... | me:1, 2, k+2, 2k+2,... |
  void AscPositiveStripe(int num_workers, int me, int* ops, int size);

  // me:0, -1, -k-1, -2k-1,... | me:1, -2, -k-2, -2k-2,... | ...
  // is the opposite of AscPositiveStripe
  void DescNegativeStripe(int num_workers, int me, int* ops, int size);

  // me:0, shuffle(1, k+1, 2k+1,...) | me:1, shuffle(2, k+2, 2k+2,...) | ...
  // Shuffle of AscPositiveStripe
  void ShufflePositiveStripe(int num_workers, int me, int* ops, int size);

  // me:0, suffle(-1, -k-1, -2k-1,...) | me:1, shuffle(-2, -k-2, -2k-2,...) |
  // Shuffle of DescNegativeStripe
  void ShuffleNegativeStripe(int num_workers, int me, int* ops, int size);
};

} // namespace lock_free

#endif // MCP_LOCK_FREE_OP_GENERATOR_HEADER
