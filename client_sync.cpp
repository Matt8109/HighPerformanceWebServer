#include <cstdlib>   // exit()
#include <iostream>

#include "callback.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "http_service.hpp"
#include "http_connection.hpp"
#include "service_manager.hpp"
#include "thread.hpp"

using std::cout;
using std::endl;
using base::ServiceManager;
using base::makeThread;
using base::makeCallableOnce;
using http::HTTPClientConnection;
using http::HTTPService;
using http::Request;
using http::Response;

int main() {
  ServiceManager service(8 /*num_workers*/);
  HTTPService http_service(15000 /*admin port*/, &service);
  pthread_t tid = makeThread(makeCallableOnce(&ServiceManager::run, &service));

  HTTPClientConnection* c1;
  http_service.connect("127.0.0.1", 15001, &c1);
  HTTPClientConnection* c2;
  http_service.connect("127.0.0.1", 15001, &c2);
  if (!c1->ok() || !c2->ok()) {
    cout << "Could not connect to 127.0.0.1/15001" << endl;
    cout << "Connection 1: " << c1->errorString() << endl;
    cout << "Connection 2: " << c2->errorString() << endl;
    exit(1);
  }

  Request req1;
  Response *resp1;
  req1.method = "GET";
  req1.address = "/index.html";
  req1.version = "HTTP/1.1";
  c1->send(&req1, &resp1);

  Request req2;
  Response *resp2;
  req2.cloneFrom(req1);
  c2->send(&req2, &resp2);

  delete resp1;
  delete resp2;

  // TODO
  // missing Connection::close(), which with release() the ref
  // counted connection, causing its deleteion.

  service.stop();
  pthread_join(tid, NULL);
}
