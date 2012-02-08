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
using test::TestThread;

TEST(NoTask, Stop) {
	ThreadPool* thread_pool = new ThreadPoolNormal(CORE_COUNT);

	thread_pool->stop();

	EXPECT_EQ(thread_pool->count(), 0);

	delete thread_pool;
}

TEST(SingleTask, HitAndStop) {
  ThreadPool* thread_pool = new ThreadPoolNormal(CORE_COUNT);

	TestThread test_thread(thread_pool);

	Callback<void>* thread_method = makeCallableOnce(&TestThread::hit, &test_thread);

	thread_pool->addTask(thread_method);

	thread_pool->stop();

	EXPECT_EQ(thread_pool->count(), 0); // Make sure the pool is empty after stop
	EXPECT_TRUE(test_thread.is_hit); // Make sure the thread actually ran 

	delete thread_pool;
}

TEST(SingleTaskMultipleExecutions, Count) {
	ThreadPool* thread_pool = new ThreadPoolNormal(CORE_COUNT);

	TestThread test_thread(thread_pool);

  EXPECT_EQ(test_thread.count, 0);

	Callback<void>* thread_method = makeCallableMany(&TestThread::increase, &test_thread);

	thread_pool->addTask(thread_method); // Kick off the tasks
	thread_pool->addTask(thread_method);

	thread_pool->stop();

	EXPECT_EQ(thread_pool->count(), 0);
	EXPECT_EQ(test_thread.count, 2);
}

// Create and run a task, stop the the pool, schedule another
// task and make sure it doesnt execute
TEST(SingleTaskSingleExecution, TaskStopTask) {

}

} // unnammed namespace

int main(int argc, char* argv[]) {
  return RUN_TESTS(argc, argv);
}
