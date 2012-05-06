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

// This class is not actually lock free. It was named such only to match the
// standard set up the hash table and that which is included in the lab 5 guide
// It is an implementation of the skip list described by Herlihy, Lev, Luchangco 
// and Shavit in the paper we read for class

template<class T>
class LockFreeSkipList {
public:
  struct Node {
    long key;
    T value;
    int topLayer;
    Node** nexts;
    volatile bool marked;
    volatile bool fullyLinked;
    mutable Mutex lock;

    Node(long v, T data, int topLevel) 
      : key(v),
        value(data),
        topLayer(topLevel++),           // already 0 offset, so we want one more
        marked(false),
        fullyLinked(false) {

      nexts = new Node*[topLevel];
      
      for (int i = 0; i < topLevel; i++)
        nexts[i] = NULL;
    }
  };

  LockFreeSkipList();

  // The destructor is not thread-safe
  ~LockFreeSkipList();

  bool add(long v, T data);
  bool contains(long v);
  bool okToDelete(Node* canidate, int lFound);
  bool remove(long v);
  int findNode(long v, Node** preds, Node** succs);
  T get(long v);                                    // gets the value of a node
  void printList();

private:
  Node LSentinel;
  Node RSentinel;

  int randomLevel(int max);                         // generate lvl 1-MAX_HEIGHT
  void printList(Node* node, int level);            // prints a given list lvl
  void unlock(Node** preds, int highestLocked);     // unlocks at every lvl used
};


template <class T>
LockFreeSkipList<T>::LockFreeSkipList() 
    : LSentinel(LONG_MIN, 0, MAX_HEIGHT),
      RSentinel(LONG_MAX, 0, MAX_HEIGHT) { 
  for (int i = 0; i < MAX_HEIGHT; i++)            // init sentinels
    LSentinel.nexts[i] = &RSentinel;

    srand (time(NULL));                           // randomize seed
}

template<typename T>
LockFreeSkipList<T>::~LockFreeSkipList() { }

template<typename T>
bool LockFreeSkipList<T>::add(long v, T data) {
  int topLayer = randomLevel(MAX_HEIGHT);
  Node* preds[MAX_HEIGHT];
  Node* succs[MAX_HEIGHT];

  while (true) {
    int lFound = findNode(v, preds, succs);

    if (lFound != -1) {
      Node* nodeFound = succs[lFound];

      if (!nodeFound->marked) {
        while (!(nodeFound->fullyLinked)) { }          // loop until consistent

        return false;
      }
      continue;
    }

    int highestLocked = -1;

    Node* pred = NULL;
    Node* succ = NULL;
    Node* prevPred = NULL;
    bool valid = true;

    for (int layer = 0; valid && (layer <= topLayer); layer++) {
      pred = preds[layer];
      succ = succs[layer];

      if (pred != prevPred) {
        pred->lock.lock();
        highestLocked = layer;
        prevPred = pred;
      }

      valid = !(pred->marked) && !(succ->marked) && pred->nexts[layer] == succ;

      if (!valid) {
        unlock(preds, highestLocked); 
        continue;
      }
        
      Node* newNode = new Node(v, data, topLayer);

      for (int layer = 0; layer <= topLayer; layer++) {
        newNode->nexts[layer] = succs[layer];
        preds[layer]->nexts[layer] = newNode;
      }

      newNode->fullyLinked = true;
      unlock(preds, highestLocked);
      return true;
    }
  }
}

template<typename T>
bool LockFreeSkipList<T>::contains(long v) {
  Node* preds[MAX_HEIGHT];
  Node* succs[MAX_HEIGHT];

  int lFound = findNode(v, preds, succs);

  return lFound != -1 && succs[lFound]->fullyLinked 
           && !(succs[lFound]->marked);
}

template<typename T>
T LockFreeSkipList<T>::get(long v) {

  int lFound = -1;
  Node* pred = &LSentinel;

  for (int layer = MAX_HEIGHT - 1; layer >= 0; layer--) {
    Node* curr = pred->nexts[layer];

    while (v > curr->key) {
      pred = curr;
      curr = pred->nexts[layer];
    }

    if (lFound == -1 && v == curr->key) {
      return curr->value;
    }
  }

  return 0;    // didnt find what we were looking for
}

template<typename T>
bool LockFreeSkipList<T>::okToDelete(Node* candidate, int lFound) {
  return candidate->fullyLinked && candidate->topLayer == lFound 
            && !(candidate->marked);
}

template<typename T>
bool LockFreeSkipList<T>::remove(long v) { 
  Node* nodeToDelete = NULL;
  bool isMarked = false;
  int topLayer = -1;
  Node* preds[MAX_HEIGHT];
  Node* succs[MAX_HEIGHT];

  while (true) {
    int lFound = findNode(v, preds, succs);

    if (isMarked || (lFound != -1 && okToDelete(succs[lFound], lFound))) {
      if (!isMarked) {
        nodeToDelete = succs[lFound];
        topLayer = nodeToDelete->topLayer;
        nodeToDelete->lock.lock();

        if (nodeToDelete->marked) {
          nodeToDelete->lock.unlock();
          return false;
        }

        nodeToDelete->marked = true;
        isMarked = true;
      }

      int highestLocked = -1;

      Node* pred;
      Node* succ;
      Node* prevPred = NULL;
      bool valid = true;

      for (int layer = 0; valid && layer <= topLayer; layer++) {
        pred = preds[layer];
        succ = succs[layer];

        if (pred != prevPred) {
          pred->lock.lock();
          highestLocked = layer;
          prevPred = pred;
        }

        valid = !(pred->marked) && pred->nexts[layer] == succ;
      }

      if (!valid) {
        unlock(preds, highestLocked); 
        continue;
      }
        
      for (int layer = topLayer; layer >= 0; layer --) {
        preds[layer]->nexts[layer] = nodeToDelete->nexts[layer];
      }

      nodeToDelete->lock.unlock();
      unlock(preds, highestLocked);

      delete nodeToDelete;
      
      return true;
    } else {
      return false;
    }
  }
}

template<typename T>
int LockFreeSkipList<T>::findNode(long v, Node** preds, Node** succs) {
  int lFound = -1;
  Node* pred = &LSentinel;

  for (int layer = MAX_HEIGHT - 1; layer >= 0; layer--) {
    Node* curr = pred->nexts[layer];

    while (v > curr->key) {
      pred = curr;
      curr = pred->nexts[layer];
    }

    if (lFound == -1 && v == curr->key) {
      lFound = layer;
    }

    preds[layer] = pred;
    succs[layer] = curr;
  }

  return lFound;
}

template<typename T>
int LockFreeSkipList<T>::randomLevel(int max) {
  return rand() % MAX_HEIGHT;
}

template <class T>
void LockFreeSkipList<T>::unlock(Node** preds, int highestLocked) {
  for (int i = highestLocked; i >= 0; i--)
    preds[i]->lock.unlock();
}

template<typename T>
void LockFreeSkipList<T>::printList() {
  std::cout << std::endl;

  for (int layer = MAX_HEIGHT - 1; layer >= 0; layer--) 
    printList(&LSentinel, layer);

  std::cout << std::endl;
}

template <class T>
void LockFreeSkipList<T>::printList(Node* node, int level) {
  std::cout << " - " << node->key;

  if (node->nexts[level] == NULL)
    std::cout << std::endl;
  else
    printList(node->nexts[level], level);
}

}

#endif  // MCP_LOCK_FREE_SKIP_LIST_HEADER