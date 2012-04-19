#include "callback.hpp"
#include "http_connection.hpp"
#include "http_response.hpp"
#include "http_service.hpp"
#include "io_manager.hpp"

// client --local 15000 --server 10.0.1.12 --port 15001 --num_cons 500
// --rate 500 --num-calls 10

using base::AcceptCallback;
using base::Callback;
using base::IOManager;
using base::ServiceManager;
using base::makeCallableOnce;
using base::makeCallableMany;
using http::ConnectCallback;
using http::HTTPClientConnection;
using http::HTTPService;
using http::Request;
using http::Response;
using http::ResponseCallback;

class ProgressMeter {
public:
  ProgressMeter(ServiceManager* service) : service_(service) {};
  ~ProgressMeter() {}

  void check();

private:
  ServiceManager* service_; // not owned here
};

void ProgressMeter::check() {
  // TODO
  // if made progress since last time
  //   print progress
  //   reschedule
  // else
  // server->stop();
  // For now, just stop after 10 seconds.
  static int i=0;
  if (++i == 10) {
    std::cout << "stop" << std::endl;
    service_->stop();
    return;
  }

  std::cout << "." << std::flush;

  Callback<void>* cb = makeCallableOnce(&ProgressMeter::check, this);
  IOManager* io_manager = service_->io_manager();
  io_manager->addTimer(1.0 /*sec*/ , cb);
}

class Client {
public:
  Client();
  ~Client();

  void start(HTTPClientConnection* conn);
  void doRequest();
  void requestDone(Response* response);

private:
  HTTPClientConnection* conn_;
  Callback<void>*       request_cb_;   // owned here
  ResponseCallback*     response_cb_;  // owned here

  // non-copyable, non-assignable
  Client(const Client&);
  Client& operator=(const Client&);
};

Client::Client() : conn_(NULL), request_cb_(NULL), response_cb_(NULL) {
}

Client::~Client() {
  delete request_cb_;
  delete response_cb_;
}

void Client::start(HTTPClientConnection* conn) {
  if (! conn->ok()) {
    LOG(LogMessage::ERROR) << "Cannot connect: " << conn->errorString();
    return;
  }

  conn_ = conn;
  request_cb_ = makeCallableMany(&Client::doRequest, this);
  response_cb_ = makeCallableMany(&Client::requestDone, this);

  // Issue the first request. acquire() here is necessary because
  // doRequest() always release()s. And that makes it possible for it
  // to be scheduled using a timer.
  conn_->acquire();
  doRequest();
}

void Client::doRequest() {
  if (! conn_->ok()) {
    LOG(LogMessage::ERROR) << "In new request: " << conn_->errorString();

  } else {
    http::Request request;
    request.method = "GET";
    request.address = "/index.html";
    request.version = "HTTP/1.1";
    conn_->asyncSend(&request, response_cb_);
  }
  conn_->release();
}

void Client::requestDone(Response* response) {
  if (! conn_->ok()) {
    LOG(LogMessage::ERROR) << "In requestDone: " << conn_->errorString();
    return;
  }

  conn_->acquire();
  IOManager* io_manager = conn_->io_manager();
  io_manager->addTimer(0.005 /*sec*/, request_cb_);
}

int main(int argc, char* argv[]) {
  // Even if using just client calls, we set up a service. Client calls
  // will use the service's io_manager and use it to collect stats
  // automatically.
  ServiceManager service(8 /* num_workers */);
  HTTPService http_service(15000, &service);

  // Open and "fire" N parallel clients.
  const int parallel = 32;
  Client clients[parallel];
  for (int i=0; i<parallel; ++i) {
    ConnectCallback* connect_cb = makeCallableOnce(&Client::start, &clients[i]);
    http_service.asyncConnect("127.0.0.1", 15001, connect_cb);
  }

  // Launch a progress meter in the background. If things hang, let
  // the meter kill this process.
  ProgressMeter meter(&service);
  Callback<void>* progress_cb = makeCallableOnce(&ProgressMeter::check, &meter);
  IOManager* io_manager = service.io_manager();
  io_manager->addTimer(1.0 /*sec*/ , progress_cb);

  // We'd need a '/quit' request to break out of the server.
  // TODO have SIGINT break out nicely as well.
  service.run();

  return 0;
}
