#include <iostream>

#include "callback.hpp"
#include "lock.hpp"
#include "thread.hpp"

using std::cout;
using std::endl;
using base::Callback;
using base::makeCallableOnce;
using base::makeThread;
using base::Notification;

class Tester {
public:
  Tester(Notification* n) : n_(n) {}
  ~Tester() {}

  void setOther(pthread_t tid) {
    tid_other_ = tid;
  }

  void go() {
    n_->wait();
    int ret = pthread_join(tid_other_, NULL);
    cout << "ret = " << ret << endl;
  }

private:
  Notification* n_;
  pthread_t tid_other_;
};

int main() {

  for (int i =0; i < 100; i++) {
    pthread_t tids[2];
    Notification go[2];
    Tester* testers[2];

    for (int i = 0; i < 2; ++i) {
      testers[i] = new Tester(&go[i]);
      Callback<void>* cb = makeCallableOnce(&Tester::go, testers[i]);
      tids[i] = makeThread(cb);
    }

    for (int i = 0; i < 2; ++i) {
      testers[i]->setOther(tids[1-i]);
      go[i].notify();
    }

    for (int i = 0; i < 2; ++i) {
      pthread_join(tids[i], NULL);
    }

    for (int i = 0; i < 2; ++i) {
      delete testers[i];
    }
}

  return 0;
}
