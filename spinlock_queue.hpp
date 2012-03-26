#ifndef DISTRIB_BASE_SPINLOCK_HEADER
#define DISTRIB_BASE_SPINLOCK_HEADER

namespace base {

struct Node {
  Node()
      : locked(true),
        next(NULL) { 
 bool locked;
 Node* next;
};

class Spinlock {
public:
  void lock() {
    Node* previous;
    Node* temp = new Node();

    // essentially atomic swap
    previous = __sync_lock_test_and_set(tail, temp);

    if (!previous) {
      previous->next = temp;
      while (temp->locked)
    }
  }

  void unlock() {
    Node* current = tail;

    while (current->locked) {   // loop until we find the running node 
      while (!current->next);   // wait if the list inconsistent

      current = current->next;
    }

    

  }

private:
  int locked_;
  Node* tail;

  // We force a memory access to 'locked_' without any barrier
  // guarantees. (This would equate to a load with
  // 'memory_order_relaxed'.)
  int loadLockState() const volatile {
    return locked_;
  }

  // Non-copyable, non-assignable
  Spinlock(Spinlock&);
  Spinlock& operator=(Spinlock&);
}; 

};

}

#endif // DISTRIB_BASE_SPINLOCK_HEADER