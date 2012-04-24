#ifndef MCP_LOCK_FREE_SKIP_LIST_HEADER
#define MCP_LOCK_FREE_SKIP_LIST_HEADER

#define MAX_HEIGHT 10

#include <cstdlib>
#include <limits.h>

#include "spinlock.hpp"

using base::Spinlock;

namespace lock_free {

struct Node {
	long key;
	int topLayer;
	Node** nexts;
	bool marked;
	bool fullyLinked;
	Spinlock lock;

  Node(long v, int topLevel) 
    : key(v),
      topLayer(topLevel),
      nexts(new Node*[topLevel]),
      marked(false),
      fullyLinked(false) {
    for (int i = 0; i < topLevel; i++)
      nexts[i] = NULL;
  }
};

class LockFreeSkipList {
public:
  explicit LockFreeSkipList();

  // The destructor is not thread-safe
  ~LockFreeSkipList();

  bool Add(long v);
  bool Contains(long v);
  bool OkToDelete(Node* canidate, int lFound);
  bool Remove(long v);
  int FindNode(long v, Node** preds, Node** succs);

private:
  Node LSentinel;
  Node RSentinel;

  int RandomLevel(int max);                         // generate lvl 1-MAX_HEIGHT
  void Unlock(Node** preds, int highestLocked);     // unlocks at every lvl used
};

}

#endif  // MCP_LOCK_FREE_SKIP_LIST_HEADER