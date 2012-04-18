#ifndef MCP_LOCK_FREE_LIST_HEADER
#define MCP_LOCK_FREE_LIST_HEADER

#include "hazard_pointers.hpp"
#include "logging.hpp"
#include "markable_pointer.hpp"
#include "thread_pool_fast.hpp"

namespace lock_free {

using base::ThreadPoolFast;

// This is a list-based, lock-free set based on the article "Hazard
// Pointers: Safe Memory Reclamation for Lock-Free Objects," by Maged
// Michael, in IEEE ToPDS, 15(6), June 2004.
//
// The corner-stones of the list are (a) that it uses logical deletion
// for nodes and (b) that it does not reclaim a logically deleted
// until there's no active reference to that node (e.g., a reading
// thread traversing a node that's just been delete). Please, refer to
// the article for more details.
//
// The class is thread safe but its destructor. We expect no
// concurrent access when the list is being taken down. Otherwise, to
// achieve thread-safety we placed the proper barrier instructions
// (memory fences) in the code.
//
// Notes on placement of barrier instructions:
//
//   + We want to establish a total order when it comes to two
//     specific operations: the reclaiming of a node, which can't be
//     done if there are active references, and the stating that an
//     actual reference is active. We make sure either of these
//     "sides" are done with full barrier semantics. (Recall that a
//     barrier is intended to synchronize memory accesses with another
//     barrier, so it is easy to think of this as having two sides.)
//
//   + On the active reference side, every time we visit a node, we
//     take a hazard pointer against that node. We put a barrier at
//     each such occasion.  On the reclaiming side, physically
//     unlinking a node from the list is done atomically, and with a
//     full barrier as well. It's easy by visualizing this as the
//     follows. The left side is the reading side. The right, the
//     reclaiming.
//
//               reading logic   |   reclaiming logic
//                               |
//              hp[0] = cur;     |
//              barrier       ===|===  if CAS(prev, cur, next)
//              test prev->cur   |        retire node
//                               |
//
//   + Either the left barrier is executed first or the right CAS
//     is executed first. If the barrier executes first, then the
//     retire node logic will consider 'cur' as an active reference
//     and will not reclaim it (though the logic, not shown here,
//     would have marked that node as deleted logically). If the
//     CAS is executed first, the prev->next pointer in the list
//     wont point to cur anymore, and the reading logic must
//     restart
//
template<typename T>
class LockFreeList {
public:

  // All access to 'next' should consider the memory ordering
  // they require. Right now, if we manipulate 'next' without an
  // atomic instruction with full barrier semantics, we implicitly
  // are saying that if the access is relaxed, we're okay with it.
  struct Node {
    T     data;
    Node* next;  // treat as atomic type

    Node() : data(), next(NULL) {}
  };

  // Creates a list and prepares it to be accessed by num_threads,
  // whose 'ME()' ids would range from 0..num_threads-1. A
  // ThreadPool's worker threads would comply.
  explicit LockFreeList(int num_threads);

  // The destructor is not thread-safe
  ~LockFreeList();

  // Returns true if 'data' does not yet exist on the set and inserts
  // it. Otherwise returns false.
  bool insert(const T& data);

  // Returns true if 'data' exists on the set and remove it. Otherwise
  // returns false.
  bool remove(const T& data);

  // Returns true if 'data' exists on the set.
  bool lookup(const T& data);

  // Returns true after traversing the list, making sure the ordering
  // of elements is preserved. Otherwise returns false.
  bool checkIntegrity() const;

private:
  // Memory ownership is either the list's, ie. the chain of nodes
  // started by 'head_', or the hazard pointers structures. Marking a
  // node as being used does *not* transfer ownership. What does is
  // unlinking. Notice that every code site that unlinks a node
  // immediately calls the retiring logic of the hazards
  // pointers. *That* implies ownership transfer.
  Node*                  head_;
  HazardPointers<Node,2> hps_;

  // Returns true and fills in 'ctx' (prev, cur, next pointers) if a
  // node with 'data' exists. Otherwise returns false. 'ctx->cur' in
  // that case will point to the node immediately after the 'data'
  // would be. 'ctx->prev' and 'ctx->next' would be the surrounding
  // nodes.
  struct LookupContext;
  bool lookupInternal(Node** start, const T& data, LookupContext* ctx);

  // Non-copyable, non-assignable.
  LockFreeList(LockFreeList&);
  LockFreeList& operator=(const LockFreeList&);
};

template<typename T>
struct LockFreeList<T>::LookupContext{
  Node** prev;  // treat as atomic type
  Node* cur;
  Node* next;

  LookupContext() : prev(NULL), cur(NULL), next(NULL) {}
};

template<typename T>
LockFreeList<T>::LockFreeList(int num_threads)
  : head_(NULL), hps_(num_threads) {
}

template<typename T>
LockFreeList<T>::~LockFreeList() {
  while (head_ != NULL) {
    Node* hold = head_;
    head_ = head_->next;
    delete hold;
  }
}

template<typename T>
bool LockFreeList<T>::insert(const T& key) {
  Node* new_node = new Node;
  new_node->data = key;

  while (true) {
    LookupContext ctx;
    if (lookupInternal(&head_, key, &ctx)) {
      delete new_node;
      return false;
    }

    new_node->next = ctx.cur;
    if (__sync_bool_compare_and_swap(ctx.prev, ctx.cur, new_node)) {
      return true;
    }
  }
}

template<typename T>
bool LockFreeList<T>::remove(const T& key) {
  while (true) {
    LookupContext ctx;
    if (! lookupInternal(&head_, key, &ctx)) {
      return false;
    }

    // Mark the node logically deleted.
    Node* next_marked = MarkablePointer<Node>::mark(ctx.next);
    if (! __sync_bool_compare_and_swap(&ctx.cur->next, ctx.next, next_marked)) {
      continue;
    }

    // Try a physical deletion.
    if (__sync_bool_compare_and_swap(ctx.prev, ctx.cur, ctx.next)) {
      hps_.retireNode(ThreadPoolFast::ME(), ctx.cur);
    } else {
      lookupInternal(&head_, key, &ctx);
    }
    return true;
  }
}

template<typename T>
bool LockFreeList<T>::lookup(const T& key) {
  LookupContext ctx;
  return lookupInternal(&head_, key, &ctx);
}

template<typename T>
bool LockFreeList<T>::lookupInternal(Node** start,
                                     const T& key,
                                     LookupContext* ctx) {
  // Mere abbreviation, for readability.
  Node**& prev = ctx->prev;
  Node*& cur = ctx->cur;
  Node*& next = ctx->next;

  Node** hp = hps_.getHPRec(ThreadPoolFast::ME());

try_again:
  // Skip the sentinel.
  prev = start;
  cur = *prev;
  while (cur != NULL) {

    hp[0] = cur;
    __sync_synchronize();

    if (*prev != cur) {
      goto try_again;
    }
    next = cur->next;

    if (MarkablePointer<Node>::isMarked(next)) {
      Node* unmarked_next = MarkablePointer<Node>::unmark(next);
      if (! __sync_bool_compare_and_swap(prev, cur, unmarked_next)) {
        goto try_again;
      }
      hps_.retireNode(ThreadPoolFast::ME(), cur);
      cur = unmarked_next;

    } else {
      T cur_key = cur->data;
      if (*prev != cur) {
        goto try_again;
      }
      if (cur_key >= key) {
        return (cur_key == key);
      }

      prev = &cur->next;
      Node* temp = hp[0];
      hp[0] = hp[1];
      hp[1] = temp;
      __sync_synchronize();

      cur = next;
    }
  }
  return false;
}

template<typename T>
bool LockFreeList<T>::checkIntegrity() const {
  if (head_ == NULL) {
    return true;
  }

  T* prev_element = &head_->data;
  Node* cur = head_->next;
  while (cur != NULL) {
    if (*prev_element >= cur->data) {
      LOG(LogMessage::ERROR) << " Error in list integrity at " << this
                             << " previous greater than current: "
                             << prev_element << " / " << cur->data;
      return false;
    }
    prev_element = &cur->data;
    cur = cur->next;
  }
  return true;
}

} // namespace lock_free

#endif  // MCP_LOCK_FREE_LIST_HEADER
