#include <algorithm>

#include "op_generator.hpp"

namespace lock_free {

using std::random_shuffle;

void OpGenerator::AscPositiveStripe(int num_workers,
                                    int me,
                                    int* ops,
                                    int size) {
  for (int i=0; i<size; i++) {
    ops[i] = num_workers*i + me + 1;
  }
}

void OpGenerator::DescNegativeStripe(int num_workers,
                                     int me,
                                     int* ops,
                                     int size) {
  for (int i=0; i<size; i++) {
    ops[i] = - (num_workers*i+ me + 1);
  }
}

void OpGenerator::ShufflePositiveStripe(int num_workers,
                                        int me,
                                        int* ops,
                                        int size) {
  for (int i=0; i<size; i++) {
    ops[i] = num_workers*i + me + 1;
  }
  random_shuffle(ops, ops + size);
}

void OpGenerator::ShuffleNegativeStripe(int num_workers,
                                        int me,
                                        int* ops,
                                        int size) {
  for (int i=0; i<size; i++) {
    ops[i] = - (num_workers*i + me + 1);
  }
  random_shuffle(ops, ops + size);
}

}  // namespace lock_free
