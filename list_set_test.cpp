// Code copyright Alberto Lerner and Matthew Mancuso
// See git blame for details

#include <stdlib.h>
#include <pthread.h>
#include "list_set.hpp"
#include "test_unit.hpp"

namespace {

using base::ListBasedSet;

void* insertion_function(void* temp);
void* deletion_function(void* temp);

struct Params {
  bool isEven;
  ListBasedSet* set;
};

TEST(Simple, Insertion) {
  ListBasedSet s;
  EXPECT_TRUE(s.insert(99));
}

//test the clear function
TEST(Simple, Clear) {
  ListBasedSet s; 
  
  s.insert(5);

  s.clear();

  EXPECT_FALSE(s.remove(5));
  EXPECT_TRUE(s.checkIntegrity());
}

//make sure the lookup function works
TEST(Simple, Lookup) {
  ListBasedSet s;

  s.insert(4);
  s.insert(5);

  EXPECT_TRUE(s.lookup(4));
  EXPECT_TRUE(s.lookup(5));
  EXPECT_FALSE(s.lookup(6));
  EXPECT_TRUE(s.checkIntegrity());
}

//testing the integrity when values
//are inserted out of order
TEST(MultipleChanges, OutOfOrderInsert) {
  ListBasedSet s;
  
  s.insert(1);
  s.insert(4);
  s.insert(3);
 
  EXPECT_TRUE(s.checkIntegrity());
}
 
//test multiple thread's inserting at
//the same time still produces a valid 
//state in the set
TEST(ThreadSafety, TwoInsertionThreads) {
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

  EXPECT_TRUE(s.lookup(1));
  EXPECT_TRUE(s.lookup(6))
  EXPECT_FALSE(s.lookup(399));
  EXPECT_TRUE(s.checkIntegrity());
}

//Tests one thread inserting, while the onther
//is deleting data
TEST(ThreadSafety, InsertionAndDeletion)
{
  ListBasedSet s;
  pthread_t insertThread, deleteThread;
  Params insertParams, deleteParams;

  insertParams.isEven = true;
  insertParams.set = &s;

  deleteParams.set = &s;

  pthread_create(&insertThread, NULL, insertion_function, (void*) &insertParams);
  pthread_create(&deleteThread, NULL, deletion_function, (void*) &deleteParams);

  pthread_join(insertThread, NULL);
  pthread_join(deleteThread, NULL);

  EXPECT_TRUE(s.checkIntegrity());
}

// tests insertions before thread, thread insertions
// deletions and post insertion and deletions
TEST(ThreadSafety, Complex)
{
  ListBasedSet s;
  pthread_t insertThread1, insertThread2, deleteThread;
  Params insertParams1, insertParams2, deleteParams;

  insertParams1.isEven = true;
  insertParams1.set = &s;

  insertParams2.isEven = false;
  insertParams2.set = &s;

  deleteParams.set = &s;

  //add a few started values
  s.insert(1);
  s.insert(4);
  s.insert(499);

  pthread_create(&insertThread1, NULL, insertion_function, (void*) &insertParams1);
  pthread_create(&insertThread2, NULL, insertion_function, (void*) &insertParams2);
  pthread_create(&deleteThread, NULL, deletion_function, (void*) &deleteParams);

  pthread_join(insertThread1, NULL);
  pthread_join(insertThread2, NULL);
  pthread_join(deleteThread, NULL);

  //add a few more elements after the whole process
  s.insert(300);
  s.insert(500);
  
  EXPECT_FALSE(s.lookup(499));
  EXPECT_TRUE(s.lookup(300));
  EXPECT_TRUE(s.lookup(500));
  EXPECT_TRUE(s.checkIntegrity());
}

//inserts either even or odd values into the list
//based on the parameter passed to the thread
void *insertion_function(void* temp) {
  Params* params = (Params*) temp;

  int i = params->isEven ? 0 : 1;

  for (; i<100; i+=2) 
    params->set->insert(i);

  return NULL;
}

//deletes values from the list
void *deletion_function(void* temp)
{
  Params* params = (Params*) temp;
  
  //remove numbers from the set reguardless
  //of whether they have been inserted or not
  for (int i=100; i>0; i--)
    params->set->remove(i);

  return NULL;
}

} // unnamed namespace

int main(int argc, char* argv[]) {
  return RUN_TESTS(argc, argv);
}


