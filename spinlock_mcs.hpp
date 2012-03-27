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

  Node() : locked(true), next(NULL) {}
 
  bool loadLockState() const volatile {
    return locked;
  }
};

class SpinlockMcs {
public:
  SpinlockMcs()
    : tail_(NULL) { }

  void lock() {             // overload to match normal lock interface
    if (qnode_ == NULL) {
      qnode_ = new Node();

      std::cout << "Made new node" << std::endl;
    }

    lock(qnode_);
  }

  void lock(Node* node) {
    Node* previous;

    node->next = NULL;

    previous = __sync_lock_test_and_set(&tail_, node);

    if (previous != NULL) {
      node->locked = true;
      previous->next = node;
      
      while (node->loadLockState());
    } else {
      node->locked = false;
    }
  }

  void unlock() {                // overload to match normal lock interface
    unlock(qnode_);
  }

  void unlock(Node* node) {
    if (node->next == NULL) {
       if(__sync_bool_compare_and_swap(&tail_, node, NULL))
          return;

       while (node->next == NULL);
    }

    node->next->locked = false;
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
