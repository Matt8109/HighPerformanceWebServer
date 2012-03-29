#ifndef DISTRIB_BASE_SPINLOCKMCS_HEADER
#define DISTRIB_BASE_SPINLOCKMCS_HEADER

#include <cstddef>
#include <iostream>
#include <pthread.h>
#include <stdlib.h>

namespace base {

struct Node {
public:
  bool locked;
  Node* next;

  Node() : locked(false), next(NULL) {}
 
  bool loadLockState() const volatile {
    return locked;
  }

  Node* loadNextState() const volatile {
    return next;
  }
};

class SpinlockMcs {
public:
  SpinlockMcs()
    : tail_(NULL) { }

  ~SpinlockMcs() {}

  void lock() {
    Node* previous;
    if (qnode_ == NULL)
     qnode_ = new Node();
    
    qnode_->next = NULL;

    previous = __sync_lock_test_and_set(&tail_, qnode_);

    if (previous != NULL) {
      qnode_->locked = true;
      previous->next = qnode_;

      while (qnode_->loadLockState());
    }
  }

  void unlock() {
    if (qnode_->next == NULL) {
      if (__sync_bool_compare_and_swap(&tail_, qnode_, NULL))
        return;

      while (qnode_->loadNextState() == NULL);
    }

    qnode_->next->locked = false;
  }

private:
  Node* tail_;
  static __thread Node* qnode_;

  // Non-copyable, non-assignable
  SpinlockMcs(SpinlockMcs&);
  SpinlockMcs& operator=(SpinlockMcs&);
}; 

__thread Node* SpinlockMcs::qnode_;

}

#endif // DISTRIB_BASE_SPINLOCKMCS_HEADER
