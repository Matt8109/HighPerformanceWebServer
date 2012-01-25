#include <stdlib.h>
#include <pthread.h>
#include "list_set.hpp"
#include "test_unit.hpp"

namespace {

using base::ListBasedSet;

void *insertion_function(void* temp);

struct Params {
  bool isEven;
  ListBasedSet* set;
};

TEST(Simple, Insertion) {
  ListBasedSet s;
  EXPECT_TRUE(s.insert(99));
}

TEST(ThreadSafety, TwoThreads) {
  ListBasedSet s;
  pthread_t thread1, thread2;
  Params t1Params, t2Params;

  t1Params.isEven=true;
  t1Params.set = &s;

  t2Params.isEven = false;
  t2Params.set = &s;

  //create two threads, one to insert even number, and one to insert odd
  pthread_create(&thread1, NULL, insertion_function, (void*) &t1Params); 
  pthread_create(&thread2, NULL, insertion_function, (void*) &t2Params);

  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);

  EXPECT_TRUE(s.checkIntegrity());
}

void *insertion_function(void* temp) {
  Params* params = (Params*) temp;

  int i = !!params->isEven;

  for (; i<100; i+=2) 
    params->set->insert(i);

  return NULL;
}

} // unnamed namespace

int main(int argc, char* argv[]) {
  return RUN_TESTS(argc, argv);
}


