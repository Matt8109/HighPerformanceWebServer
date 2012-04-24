#ifndef MCP_LOCK_FREE_SKIP_LIST_HEADER
#define MCP_LOCK_FREE_SKIP_LIST_HEADER

#define MAX_HEIGHT 10

#include <iostream>
#include <limits.h>
#include <stdlib.h>
#include <time.h>

#include "lock.hpp"

using base::Mutex;

namespace lock_free {

struct Node {
	long key;
	int topLayer;
	Node** nexts;
	bool marked;
	bool fullyLinked;
	Mutex lock;

  Node(long v, int topLevel) 
    : key(v),
      topLayer(topLevel),
      marked(false),
      fullyLinked(false) {

    nexts = new Node*[topLevel];
    
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
  void PrintList();

private:
  Node LSentinel;
  Node RSentinel;

  int RandomLevel(int max);                         // generate lvl 1-MAX_HEIGHT
  void PrintList(Node* node, int level);            // prints a given list lvl
  void Unlock(Node** preds, int highestLocked);     // unlocks at every lvl used
};

}

#endif  // MCP_LOCK_FREE_SKIP_LIST_HEADER