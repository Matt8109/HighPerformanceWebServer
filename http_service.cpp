// Code copyright Alberto Lerner and Matthew Mancuso
// See git blame for details

#include <errno.h>
#include <string.h>  // strerror

#include "callback.hpp"
#include "http_service.hpp"
#include "http_connection.hpp"
#include "logging.hpp"

namespace http {

using base::Callback;
using base::makeCallableMany;
using base::makeCallableOnce;

HTTPService::HTTPService(int port, ServiceManager* service_manager)
  : service_manager_(service_manager),
    stats_(service_manager->num_workers()),
    file_cache_(50<<20 /* 50MB */) {
  AcceptCallback* cb = makeCallableMany(&HTTPService::acceptConnection, this);
  service_manager_->registerAcceptor(port, cb /* ownership xfer */);
}

HTTPService::~HTTPService() {
}

void HTTPService::stop() {
  service_manager_->stop();
}

void HTTPService::acceptConnection(int client_fd) {
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
  new HTTPServerConnection(this, client_fd);
}

void HTTPService::asyncConnect(const string& host,
                               int port,
                               ConnectCallback* cb) {
  if (service_manager_->stopped()) {
    return;
  }

  HTTPClientConnection* conn = new HTTPClientConnection(this);
  conn->connect(host, port, cb);
}

void HTTPService::connect(const string& host,
                          int port,
                          HTTPClientConnection** conn) {
  Notification n;
  ConnectCallback* cb = makeCallableOnce(&HTTPService::connectInternal,
                                         this,
                                         &n,
                                         conn);
  asyncConnect(host, port, cb);
  n.wait();
}

void HTTPService::connectInternal(Notification* n,
                                  HTTPClientConnection** user_conn,
                                  HTTPClientConnection* new_conn) {
  *user_conn = new_conn;
  n->notify();
}

} // namespace http
