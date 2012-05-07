#include <errno.h>
#include <string.h>  // strerror

#include "callback.hpp"
#include "kv_connection.hpp"
#include "kv_service.hpp"
#include "logging.hpp"

namespace kv {

using base::Callback;
using base::makeCallableMany;
using base::makeCallableOnce;

KVService::KVService(int port, ServiceManager* service_manager)
  : service_manager_(service_manager) {
    AcceptCallback* cb = makeCallableMany(&KVService::acceptConnection, this);
    service_manager_->registerAcceptor(port, cb /* ownership xfer */);
}

KVService::~KVService() {
}

void KVService::stop() { 
  service_manager_->stop();
}

void KVService::acceptConnection(int client_fd) {
  if (service_manager_->stopped()) {
    return;
  }

  if (client_fd < 0) {
    LOG(LogMessage::ERROR) << "Error accepting: " << strerror(errno);
    service_manager_->stop();
    return;
  }

  // The client will be destroyed if the peer closes the socket(). If
  // the server is stopped, all the connections leak. But the
  // sockets will be closed by the process termination anyway.
  new KVServerConnection(this, client_fd);
}

void KVService::asyncConnect(const string& host,
                               int port,
                               ConnectCallback* cb) {
  if (service_manager_->stopped()) {
    return;
  }

  KVClientConnection* conn = new KVClientConnection(this);
  conn->connect(host, port, cb);
}

void KVService::connect(const string& host,
                          int port,
                          KVClientConnection** conn) {
  Notification n;
  ConnectCallback* cb = makeCallableOnce(&KVService::connectInternal,
                                         this,
                                         &n,
                                         conn);
  asyncConnect(host, port, cb);
  n.wait();
}

void KVService::connectInternal(Notification* n,
                                  KVClientConnection** user_conn,
                                  KVClientConnection* new_conn) {
  *user_conn = new_conn;
  n->notify();
}

} // namespace kv
