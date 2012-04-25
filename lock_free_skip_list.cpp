#include "lock_free_skip_list.hpp"

namespace lock_free {

LockFreeSkipList::LockFreeSkipList() 
    : LSentinel(LONG_MIN, MAX_HEIGHT),
      RSentinel(LONG_MAX, MAX_HEIGHT) { 
  for (int i = 0; i < MAX_HEIGHT; i++)            // init sentinels
    LSentinel.nexts[i] = &RSentinel;

    srand (time(NULL));                           // randomize seed
}

LockFreeSkipList::~LockFreeSkipList() { }

bool LockFreeSkipList::Add(long v) {
  int topLayer = RandomLevel(MAX_HEIGHT);
  Node* preds[MAX_HEIGHT];
  Node* succs[MAX_HEIGHT];

  while (true) {
    int lFound = FindNode(v, preds, succs);

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
        Unlock(preds, highestLocked); 
        continue;
      }
        
      Node* newNode = new Node(v, topLayer);

      for (int layer = 0; layer <= topLayer; layer++) {
        newNode->nexts[layer] = succs[layer];
        preds[layer]->nexts[layer] = newNode;
      }

      newNode->fullyLinked = true;
      Unlock(preds, highestLocked);
      return true;
    }
  }
}

bool LockFreeSkipList::Contains(long v) {
  Node* preds[MAX_HEIGHT];
  Node* succs[MAX_HEIGHT];

  int lFound = FindNode(v, preds, succs);

  return lFound != -1 && succs[lFound]->fullyLinked 
           && !(succs[lFound]->marked);
}

bool LockFreeSkipList::OkToDelete(Node* candidate, int lFound) {
  return candidate->fullyLinked && candidate->topLayer == lFound 
            && !(candidate->marked);
}

bool LockFreeSkipList::Remove(long v) { 
  Node* nodeToDelete = NULL;
  bool isMarked = false;
  int topLayer = -1;
  Node* preds[MAX_HEIGHT];
  Node* succs[MAX_HEIGHT];

  while (true) {
    int lFound = FindNode(v, preds, succs);

    if (isMarked || (lFound != -1 && OkToDelete(succs[lFound], lFound))) {
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
        Unlock(preds, highestLocked); 
        continue;
      }
        
      for (int layer = topLayer; layer >= 0; layer --) {
        preds[layer]->nexts[layer] = nodeToDelete->nexts[layer];
      }

      nodeToDelete->lock.unlock();
      Unlock(preds, highestLocked);
      return true;
    } else {
      return false;
    }
  }
}

int LockFreeSkipList::FindNode(long v, Node** preds, Node** succs) {
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

int LockFreeSkipList::RandomLevel(int max) {
 // return 9;

  return rand() % MAX_HEIGHT;
}

void LockFreeSkipList::Unlock(Node** preds, int highestLocked) {
  for (int i = highestLocked; i >= 0; i--)
    preds[i]->lock.unlock();
}

void LockFreeSkipList::PrintList() {
  std::cout << std::endl;

  for (int layer = MAX_HEIGHT - 1; layer >= 0; layer--) 
    PrintList(&LSentinel, layer);

  std::cout << std::endl;
}

void LockFreeSkipList::PrintList(Node* node, int level) {
  std::cout << " - " << node->key;

  if (node->nexts[level] == NULL)
    std::cout << std::endl;
  else
    PrintList(node->nexts[level], level);
}

}