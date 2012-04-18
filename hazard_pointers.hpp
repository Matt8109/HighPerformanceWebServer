#ifndef MCP_LOCK_FREE_HAZARD_POINTER_HEADER
#define MCP_LOCK_FREE_HAZARD_POINTER_HEADER

#include <cstring> // size_t
#include <inttypes.h>

#include <tr1/unordered_set>

#include "cpu_arch.hpp"

namespace lock_free {

using std::tr1::unordered_set;
using base::CacheArch;

// The techniques here are based on the article "Hazard Pointers: Safe
// Memory Reclamation for Lock-Free Objects", by Maged Michael, in
// IEEE ToPDS, 15(6), June 2004.
//
// The goal of this class is to maintain a record of which portions of
// a given thread-safe data structure are currently being
// traversed/altered. Using hazard pointers alone doesn't make a
// structure thread-safe. The hazard pointer information is used to
// implement safe memory reclamation and ABA preventions.
//
// We make a few simplifying assumptions
//   + The number of threads is fixed (small) and known a priori
//   + Each thread is numbered from 0..NUM_THREADS-1, so we can use it
//     as index
//
// Thread-safety:
//   The class is thread safe in that each thread would be manipulting
//   its own portion of the structure. But each thread must know its
//   ID and that ID has to be numbered as stated above.
//
//   'retireNode()' traverses all shared strucutres but it is
//   resilient to reading out-of-date data. (It'd just cause node
//   reclamation to take longer but wouldnt reclaim a node
//   accidentally that would still be used.)
//
//   The destructor is *not* thread-safe. It assumes that all activity
//   on the data structure whose pointers are stored here has stopped.
//
// For an usage example see: lock_free_list.hpp
//
template<typename T, int NUM_PTRS>
class HazardPointers {
public:
  // Initializes internal state to keep 'num_threads' times 'NUM_PTRS'
  // pointers to 'T' nodes. Each thread accessing this class's methods
  // would identify itself by an id, from 0..num_threads-1 and each
  // thread will use only one of the IDs.
  explicit HazardPointers(int num_threads);

  // Expects all threads that manipulate the pointers to 'T' nodes to
  // be stopped. Can be called from any thread, including one that was
  // not within one of the 0..num_thread ones.
  ~HazardPointers();

  // Retuns the pointer the array of T*'s that the thread 'thread_num'
  // should use to record its hazard pointers. It is up to the
  // caller's to see to it that it only accessses within the bounds of
  // T*[0..NUM_PTRS-1].
  //
  // Assigning a pointer to an entry here does *not* transfer
  // ownership. The semantics of such assignment are: "I'm using this
  // pointer" with the obvious implications in memory reclamation.
  T** getHPRec(int thread_num);

  // Records that thread 'thread_num' wants to retire 'node' whenever
  // that becomes safe and occasionally free previously retired nodes
  // that bacame safe. Returns the number of nodes reclaimed in this
  // call (mostly for testing purposes).
  //
  // It is up to the caller to call retireNode for a given node only
  // once.
  //
  // There *is* transfer of ownership here.  The caller must have pulled
  // the 'node' apart from the structure it belonged to. Note that
  // only one of the caller's threads my retire a given 'node'.
  int retireNode(int thread_num, T* node);
  // void retireThread(int thread_num);

  //
  // Below exposed for testing purposes only. Treat as private.
  //

  // Threshold above which we trigger the search for which nodes in a
  // retired list can be reclaimend.
  static /* const */ size_t MAX_RETIRED_NODES_PER_THREAD;

  // Performs a scan in 'thread_num's retired node list and check
  // which 'Node's, if any, could be reclaimed. Returns the number of
  // node reclaimed.
  int maybeFreeNodes(int thread_num);

private:
  const int num_threads_;

  // An array of hazard pointers, K of them per thread. Our
  // simplifying assumption is that we know the number of threads a
  // priori.
  //
  // Each thread will only write to its K hazard pointers.
  // thread 0: HP[0].hazard_pointers[0..K-1]
  // thread 1: HP[1].hazard_pointers[0..K-1]
  // ...
  // But a thread may read all others' pointers to decide if it may
  // reclaim a given Node.
  //
  // Each 'hazard_pointer' array is aligned on cache line boundary. So
  // accesses from different threads won't cause false sharing.  The
  // pointer 'hold_hp_recs_' is the underlying memory, including the
  // extra chunk allocated so that HPRec items were
  // aligned. 'hold_hp_recs_' is owned here.
  //
  // The 'hazard_pointers' are *not* owned here. So storing a pointer
  // in this structure does not transfer ownership.
  struct HPRec {
    T* hazard_pointers[NUM_PTRS];  // not owned here
    char pad[CacheArch::LINE_SIZE - sizeof(hazard_pointers)];
  };
  HPRec* hp_recs_;                 // pointer into to hold_hp_recs_
  char* hold_hp_recs_;             // owned here

  // We keep one "retired list" per thread. A node in a retired list
  // is one that wants to be reclaimed, whenever it becomes safe to do
  // so. It won't be safe before all threads stop manipulating that
  // node.
  typedef unordered_set<T*> RetiredList;
  RetiredList* retired_lists_;

  // Non-copyable, non-assignable
  HazardPointers(const HazardPointers&);
  HazardPointers& operator=(const HazardPointers&);
};

template<typename T, int NUM_PTRS>
size_t HazardPointers<T, NUM_PTRS>::MAX_RETIRED_NODES_PER_THREAD = 10;

template<typename T, int NUM_PTRS>
HazardPointers<T, NUM_PTRS>::HazardPointers(int num_threads)
  : num_threads_(num_threads) {

  // We want to place each HPRec in the beginning of a cache line. So
  // we allocate extra space and compute where the next aligned
  // address starts.
  hold_hp_recs_ = new char[(num_threads_ + 1) * sizeof(HPRec)];
  uint64_t start = reinterpret_cast<uint64_t>(hold_hp_recs_)
                   % CacheArch::LINE_SIZE;
  if (start > 0) {
    hp_recs_ = reinterpret_cast<HPRec*>(hold_hp_recs_
                                        + CacheArch::LINE_SIZE
                                        - start);
  } else {
    hp_recs_ = reinterpret_cast<HPRec*>(hold_hp_recs_);
  }
  new(hp_recs_) HPRec[num_threads_];

  retired_lists_ = new RetiredList[num_threads_];
}

template<typename T, int NUM_PTRS>
HazardPointers<T, NUM_PTRS>::~HazardPointers() {
  // Recall we do not need to reclaim hazard pointers, since assigning
  // to them does not transfer ownership. We can get rid of the
  // underlying hold_hp_recs_ directly.
  delete [] hold_hp_recs_;

  // Now, nodes in the retired list are owned here. We assume that no
  // one is touching the structure these nodes retire from anymore. So
  // the following would be tantamount to running maybeFreeNodes with
  // all hazard pointer being NULL.
  for (int i=0; i<num_threads_; i++) {
    RetiredList& l = retired_lists_[i];
    for (typename RetiredList::iterator it = l.begin(); it!= l.end(); ++it) {
      delete *it;
    }
  }
  delete [] retired_lists_;
}

template<typename T, int NUM_PTRS>
T** HazardPointers<T, NUM_PTRS>::getHPRec(int thread_num) {
  return hp_recs_[thread_num].hazard_pointers;
}

// template<typename T, int NUM_PTRS>
// void HazardPointers<T, NUM_PTRS>::retireThread(int thread_num) {
//   HPRec& hp_rec = hp_recs_[thread_num];
//   for (int i=0; i<NUM_PTRS; i++) {
//     retireNode(thread_num, hp_rec.hazard_pointers[i]);
//   }
// }

template<typename T, int NUM_PTRS>
int HazardPointers<T, NUM_PTRS>::retireNode(int thread_num,
                                             T* node) {
  retired_lists_[thread_num].insert(node);
  if (retired_lists_[thread_num].size() > MAX_RETIRED_NODES_PER_THREAD) {
    return maybeFreeNodes(thread_num);
  }
  return 0;
}

template<typename T, int NUM_PTRS>
int HazardPointers<T, NUM_PTRS>::maybeFreeNodes(int thread_num) {
  int reclaimed = 0;

  // Grab a snapshot of the nodes in use.
  unordered_set<T*> active_list(num_threads_ * NUM_PTRS);
  for (int i=0; i<num_threads_; i++) {
    HPRec& hp_rec = hp_recs_[i];
    for(int j=0; j<NUM_PTRS; j++) {
      if (hp_rec.hazard_pointers[j] != NULL) {
        active_list.insert(hp_rec.hazard_pointers[j]);
      }
    }
  }

  // If any node in the local retired list is not considered hazardoud
  // by other threads, reclaim the node.
  RetiredList& l = retired_lists_[thread_num];
  typename unordered_set<T*>::iterator it = l.begin();
  while (it != l.end()) {
    if (active_list.find(*it) == active_list.end()) {
      delete *it;
      l.erase(it++);
      reclaimed++;
    } else {
      ++it;
    }
  }
  return reclaimed;
}

}  // namespace lock_free

#endif  // MCP_LOCK_FREE_HAZARD_POINTER_HEADER
