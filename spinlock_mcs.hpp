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
  unsigned int thread_id;

  Node() : locked(true), next(NULL) {}
 
  bool loadLockState() const volatile {
    return locked;
  }
};

class SpinlockMcs {
public:
  SpinlockMcs()
    : tail(NULL) { }

  void lock() {
    Node* previous;
    Node* temp = new Node();

    temp->thread_id = (unsigned int)pthread_self();

    previous = __sync_lock_test_and_set(&tail, temp);

    if (previous != NULL) {
      previous->next = temp;
      
      while (temp->loadLockState());
    } else {
      temp->locked = false;
    }
  }

  void unlock() {
    Node* lock_holder = tail;

    while (lock_holder->loadLockState()) {  // traverse the queue to find unlocked node
      while (lock_holder->next == NULL);    // wait for list to become consistent 

      lock_holder = lock_holder->next;
    }

    if (lock_holder->next == NULL) {
       if(__sync_bool_compare_and_swap(&tail, lock_holder, NULL)) {
          delete lock_holder;
          return;
       }

       while (lock_holder->next == NULL);
    }

    lock_holder->next->locked = false;

    delete lock_holder;
  }

private:
  Node* tail;

  // Non-copyable, non-assignable
  SpinlockMcs(SpinlockMcs&);
  SpinlockMcs& operator=(SpinlockMcs&);
}; 

}

#endif // DISTRIB_BASE_SPINLOCKMCS_HEADER
