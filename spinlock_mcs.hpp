#ifndef DISTRIB_BASE_SPINLOCKMCS_HEADER
#define DISTRIB_BASE_SPINLOCKMCS_HEADER

#include <cstddef>
#include <iostream>
#include <pthread.h>
#include <stdlib.h>

namespace base {

struct Node {
  Node() : locked(true), next(NULL) {}

  bool volatile locked;
  unsigned int thread_id;
  Node* volatile next;
};

class SpinlockMcs {
public:
  SpinlockMcs()
    : tail(NULL),
      current(NULL) { }

  void lock() {
    Node* previous;
    Node* temp = new Node();

    temp->thread_id = (unsigned int)pthread_self();

    // essentially atomic swap
    previous = __sync_lock_test_and_set(&tail, temp);

    if (previous != NULL) {
      previous->next = temp;
      
      while (temp->locked);
    } else {
      temp->locked = false;
      current = temp;
    }
  }

  void unlock() {
    Node* temp = current;
    //std::cout << "Unlock by" << (unsigned int)pthread_self() << std::endl;

    if (current->next == NULL) {
       if(__sync_bool_compare_and_swap(&tail, current, NULL)) {
          delete temp;
          return;
       }

       while (current->next == NULL);
    }

    current = current->next;
    current->locked = false;

    delete temp;
  }

private:
  Node* volatile tail;
  Node* volatile current;

  // Non-copyable, non-assignable
  SpinlockMcs(SpinlockMcs&);
  SpinlockMcs& operator=(SpinlockMcs&);
}; 

}

#endif // DISTRIB_BASE_SPINLOCKMCS_HEADER