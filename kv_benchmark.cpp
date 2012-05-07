#include <algorithm>
#include <iostream>
#include <pthread.h>
#include <queue>
#include <string>

#include "callback.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "kv_connection.hpp"
#include "kv_service.hpp"
#include "service_manager.hpp"
#include "test_util.hpp"
#include "thread.hpp"
#include "timer.hpp"

using base::Timer;

namespace {

using base::Callback;
using base::makeCallableMany;
using base::makeCallableOnce;
using base::makeThread;
using base::ServiceManager;
using base::Timer;
using http::Request;
using http::Response;
using kv::KVClientConnection;
using kv::KVService;
using std::cout;
using std::endl;
using std::queue;
using std::random_shuffle;
using std::string;

struct Tester {
  ServiceManager service;
  KVService kv_service;
  pthread_t service_thread;

  Tester()
    : service(4),
      kv_service(15000, &service),
      service_thread(makeThread(makeCallableOnce(&ServiceManager::run, 
                                                 &service))) { 
  }

  ~Tester() { }

  void connect(int key) {
    KVClientConnection* c1;
    kv_service.connect("127.0.0.1", 15000, &c1);
    c1->acquire();
    if (!c1->ok()) {
      cout << "Could not connect to 127.0.0.1/15000" << endl;
      cout << "Connection 1: " << c1->errorString() << endl;
      exit(1);
    }

    string key_string;
    std::stringstream convt;
    convt << key;
    key_string = convt.str();

    Request request;
    Response* response;
    request.method = "GET";
    request.address = "/" + key_string;
    request.version = "HTTP/1.1";
    c1->send(&request, &response);
    c1->release();
    delete response;
  }

  void testMethod(int start, int* ops, int loop_count) {
    for (int i = start; i < start + loop_count; i++)
      connect(ops[i]);
  }

  int* createOperations(int count, bool in_order, int max) {
    int* array = new int[count];

    for (int i = 0; i < count; i++)
      array[i] = i % max;

    if (!in_order)
      random_shuffle(array, array + count);

    return array;
  }

  Timer* runTests(int thread_count, int loop_count, bool in_order, int max) {
    Timer* timer = new Timer();
    queue<pthread_t> threads;
    int* values = createOperations(loop_count * thread_count, in_order, max);

    timer->start();

    Callback<void, int, int*, int>* cb = 
        makeCallableMany(&Tester::testMethod, this);

    for (int i = 0; i < thread_count; i++) {
      Callback<void>* cb_wrapper = 
          makeCallableOnce(&Callback<void, int, int*, int>::operator(), 
                           cb,
                           loop_count / thread_count,
                           values,
                           loop_count);

      threads.push(makeThread(cb_wrapper));
    }

    for (int i = 0; i < thread_count; i++) {
      pthread_join(threads.front(), NULL);
      threads.pop();
    }

    timer->end();

    delete [] values;

    return timer;
  }

  void testStarter(int thread_one_count, 
                   int thread_two_count, 
                   int thread_three_count,
                   int loop_count,
                   bool in_order,
                   int max) {
    Timer* timer_one = runTests(thread_one_count, loop_count, in_order, max);
    Timer* timer_two = runTests(thread_two_count, loop_count, in_order, max);
    Timer* timer_three = runTests(thread_three_count, 
                                  loop_count, 
                                  in_order, 
                                  max);

    cout << timer_one->elapsed() << "  |  "
         << timer_two->elapsed() << "  |  "
         << timer_three->elapsed() 
         << endl;

    delete timer_one;
    delete timer_two;
    delete timer_three;
  }
};

} // unamed namespace

void starter(int thread_one_count, 
                   int thread_two_count, 
                   int thread_three_count,
                   int loop_count,
                   bool in_order,
                   int max) {
  Tester test;
  test.testStarter(2, 4, 10, 500, false, 30);
}

int main(int argc, char* argv[]) {
  cout << endl << "Starting benchmark, measuring 2, 4 and 6 threads." << endl;

  cout << endl << "Testing randomized insertions, with a high collision rate." 
       << endl;
  starter(2, 4, 6, 100, false, 10);

  cout << endl << endl 
       << "Testing randomized insersions, with a low collisions rate."
       << endl;
  starter(2, 4, 6, 100, false, 20);

  cout  << endl << "Overall the datastructure performs very well. Most of the "
        << endl
        << "performance slowdown is actually coming from the kv protocol"
        << endl
        << "itself performing the slow fib, which can take up to 10 percent "
        << endl
        << "of the overall time of the benchmark. In longer running code this "
        << endl
        << "would be much less significant."
        << endl << endl
        << "Specifically, the benchmark spends less then 2 percent of its "
        << endl
        << "total execution time in the skiplist and only .1 percent of the "
        << endl
        << "time waiting on a lock."
        << endl << endl
        << "The major bottleneck again, as with the last performance analysis "
        << endl
        << "is in the threadpool. This is also demonstrated by the fact the"
        << endl
        << "structure performs pretty closely whether the keyspace is highly "
        << endl
        << "clustered or not."
        << endl;
}