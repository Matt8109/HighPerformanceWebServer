#define CORE_COUNT 8 //the number of cores you have/threads to create

#include "thread_pool_normal.hpp"
#include "test_unit.hpp"
#include "test_util.hpp"
#include "callback.hpp"

namespace {

using base::Callback;
using base::ThreadPool;
using base::ThreadPoolNormal;
using base::makeCallableOnce;
using base::makeCallableMany;
using test::TestClass;

TEST(NoTask, Stop) {
	ThreadPool* thread_pool = new ThreadPoolNormal(CORE_COUNT);

	thread_pool->stop();

	EXPECT_EQ(thread_pool->count(), 0);

	delete thread_pool;
}

TEST(SingleTask, HitAndStop) {
  ThreadPool* thread_pool = new ThreadPoolNormal(CORE_COUNT);

	TestClass test_thread(thread_pool);

	Callback<void>* thread_method = 
	    makeCallableOnce(&TestClass::hit, &test_thread);

	thread_pool->addTask(thread_method);

	thread_pool->stop();

	EXPECT_EQ(thread_pool->count(), 0); // Make sure the pool is empty after stop
	EXPECT_TRUE(test_thread.is_hit); // Make sure the thread actually ran 

	delete thread_pool;
}

TEST(SingleTaskMultipleExecutions, Count) {
	ThreadPool* thread_pool = new ThreadPoolNormal(CORE_COUNT);

	TestClass test_thread(thread_pool);

  EXPECT_EQ(test_thread.count, 0);

	Callback<void>* thread_method = 
      	    makeCallableMany(&TestClass::increase, &test_thread);

	thread_pool->addTask(thread_method); // Kick off the tasks
	thread_pool->addTask(thread_method);

	thread_pool->stop();

	EXPECT_EQ(thread_pool->count(), 0);
	EXPECT_EQ(test_thread.count, 2);
	
	delete thread_pool;
}

// Create and run a task, stop the the pool, schedule another
// task and make sure it doesnt execute
TEST(SingleTaskSingleExecution, ExternalTaskStop) {
	ThreadPool* thread_pool = new ThreadPoolNormal(CORE_COUNT);

	TestClass test_thread(thread_pool);

	Callback<void>* thread_method =
		makeCallableMany(&TestClass::increase, &test_thread);

	thread_pool->addTask(thread_method);
	
	thread_pool->stop();

	thread_pool->addTask(thread_method);

	// Assuming no threads are considered to be 'waiting' if the pool is stopped
	EXPECT_EQ(thread_pool->count(), 0);
	EXPECT_EQ(test_thread.count, 1);

	delete thread_pool;
}

TEST(SingleTaskMultipleExecution, InternalTaskStop) {
	ThreadPool* thread_pool = new ThreadPoolNormal(CORE_COUNT);

	TestClass test_thread(thread_pool);

	Callback<void>* main_method =
		makeCallableMany(&TestClass::increase, &test_thread);

	Callback<void>* stop_method =
		makeCallableOnce(&TestClass::stop, &test_thread);

	for (int i=0; i< CORE_COUNT - 1; i++)  // add tasks, but want one free thread
		thread_pool->addTask(main_method);
	
	thread_pool->addTask(stop_method);
	
	for (int i=0; i<1000; i++)
		thread_pool->addTask(main_method);

	while (!thread_pool->isStopped()) { } // wait for the pool to stop

	// This probably isn't the best way to test, but essentially
	// we want to make sure that the thread pool is being stopped
	// before we reach the max possible count if all the tasks
	// had been run.
	bool less_than_max = (test_thread.count<1009) ? true : false;

	EXPECT_TRUE(less_than_max);
	EXPECT_EQ(thread_pool->count(), 0);

	delete thread_pool;
}

TEST(MultipleTasksMultipleExecutions, ExternalTaskStop) {
	ThreadPool* thread_pool = new ThreadPoolNormal(CORE_COUNT);

	TestClass test_thread(thread_pool);

	Callback<void>* count_method = 
		makeCallableMany(&TestClass::increase, &test_thread);

	Callback<void>* hit_method =
		makeCallableMany(&TestClass::flip, &test_thread);

	for (int i=0; i<99; i++)
	{
		thread_pool->addTask(count_method);
		thread_pool->addTask(hit_method);
	}

	thread_pool->stop();

	EXPECT_EQ(test_thread.count, 99);
	EXPECT_EQ(test_thread.is_hit, true);

	delete thread_pool;
}

TEST(MultipleTasksMultipleExecutions, MultipleStops) {
	// on a personal note I dont really want to have to write the code
	// that will make this pass :)
	ThreadPool* thread_pool = new ThreadPoolNormal(CORE_COUNT);

	TestClass test_thread(thread_pool);

	Callback<void>* count_method =
		makeCallableMany(&TestClass::increase, &test_thread);

	Callback<void>* stop_method =
		makeCallableOnce(&TestClass::stop, &test_thread);

	for (int i=0; i<100; i++) 
		thread_pool->addTask(count_method);
	
	// this shouldnt deadlock
	thread_pool->addTask(stop_method);
	thread_pool->stop();

	EXPECT_EQ(test_thread.count, 100);

	delete thread_pool;
}

} // unnammed namespace

int main(int argc, char* argv[]) {
  return RUN_TESTS(argc, argv);
}
