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
    : service(8),
      kv_service(15000, &service),
      service_thread(makeThread(makeCallableOnce(&ServiceManager::run, 
                                                 &service))) { 
  }

  ~Tester() { }

  void connect(int key) {
    KVClientConnection* c1;
    kv_service.connect("127.0.0.1", 15000, &c1);
    if (!c1->ok()) {
      cout << "Could not connect to 127.0.0.1/15000" << endl;
      cout << "Connection 1: " << c1->errorString() << endl;
      exit(1);
    }

    string key_string;
    std::stringstream convt;
    convt << key;
    key_string = convt.str();

    Request req1;
    Response *resp1;
    req1.method = "GET";
    req1.address = "/" + key_string;
    req1.version = "HTTP/1.1";
    c1->send(&req1, &resp1);
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

  void printTimers() {
    
  }

  Timer* runTests(int thread_count, int loop_count, bool in_order, int max) {
    Tester tester();
    Timer* timer = new Timer();
    queue<pthread_t> threads;
    Callback<void>* cb_wrapper;
    int* values = createOperations(loop_count * thread_count, in_order, max);

    timer->start();

    Callback<void, int, int*, int>* cb = 
        makeCallableMany(&Tester::testMethod, this);

    for (int i = 0; i < thread_count; i++) {
      Callback<void>* cb_wrapper = 
          makeCallableOnce(&Callback<void, int, int*, int>::operator(), 
                           cb,
                           i * loop_count,
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
         << timer_three->elapsed() << "  |  "
         << endl;

    delete timer_one;
    delete timer_two;
    delete timer_three;
  }
};

} // unamed namespace

int main(int argc, char* argv[]) {


  Tester test;
  test.connect(4);
}