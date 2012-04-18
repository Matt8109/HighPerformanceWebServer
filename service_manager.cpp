#include "service_manager.hpp"

namespace base {

ServiceManager::ServiceManager(int num_workers)
  : num_workers_(num_workers),
    io_manager_(new IOManager(num_workers)),
    stop_requested_(false),
    stopped_(false) {
 }

ServiceManager::~ServiceManager() {
  // It would be problematic if run() was still running after the
  // call to stop().  But that can't happen, for the following.  If
  // run() was called, we assume that the server will only go out of
  // scope -- what would get us here -- after run() returned, since
  // the latter blocks until a stop() is issued.  If run() wasn't
  // even called, then there's no worries overlapping that and the
  // destructor.
  //
  // After stop() completed, we can assert that
  //   + the io_manager's worker pool threads have all joined
  //   + the acceptor is stopped
  // So the destructor can safely tear down the request serving
  // machinery.
  stop();

  // Only now that there's no more callback activities and the worker
  // threads were joined, we can reckaun the acceptors. Recall, that is
  // not thread safe.
  //
  for (Acceptors::iterator it = acceptors_.begin();
       it != acceptors_.end();
       ++it) {
    delete *it;
  }
  delete io_manager_;
}

void ServiceManager::registerAcceptor(int port, AcceptCallback* cb) {
  acceptors_.push_back(new Acceptor(io_manager_, port, cb));
}

void ServiceManager::run() {
  for (Acceptors::iterator it = acceptors_.begin();
       it != acceptors_.end();
       ++it) {
    (*it)->startAccept();
  }
  io_manager_->poll();  // will block here until stop() is called

  // We must hold here until stop() completed running, which could be
  // *longer* than io_manager.poll() returning. If the destructor,
  // which could be issued after run(), kicks in, it could interfere
  // with stop().

  m_stop_.lock();
  while (! stopped_) {
    cv_stopped_.wait(&m_stop_);
  }
  m_stop_.unlock();
}

void ServiceManager::stop() {
  // Allow only one thread to actually execute the stop(). All others
  // would wait for the latter to finish, if stop() was called
  // concurrently.
  {
    ScopedLock l(&m_stop_);
    if (stop_requested_) {
      while (! stopped_) {
        cv_stopped_.wait(&m_stop_);
      }
      return;
    }
    stop_requested_ = true;
  }

  // Stop accepting new requests.
  for (Acceptors::iterator it = acceptors_.begin();
       it != acceptors_.end();
       ++it) {
    (*it)->close();
  }

  // The stopping sequence waits until all previously enqueued
  // callbacks were served. After which, the worker threads are
  // joined. The poll() call in ServiceManager::run() will be broken
  // somewhere during the stop() call.
  io_manager_->stop();

  // At this stage the internal machinery of the ServiceManager can be
  // destroyed, since all activity was guaranteed to have stopped.
  m_stop_.lock();
  stopped_ = true;
  cv_stopped_.signalAll();
  m_stop_.unlock();
}

bool ServiceManager::stopped() const {
  ScopedLock l(&m_stop_);
  return stop_requested_;
}

}  // namespace base
