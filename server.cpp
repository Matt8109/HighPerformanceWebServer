#include <iostream>
#include <sstream>

#include "acceptor.hpp"
#include "http_service.hpp"
#include "kv_service.hpp"

using base::AcceptCallback;
using base::ServiceManager;
using base::makeCallableMany;
using http::HTTPService;
using kv::KVService;

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cout << "Usage: " << argv[0] << " <port> <num-threads>" << std::endl;
    return 1;
  }

  // Parse service ports.
  int http_port;
  std::istringstream port_stream(argv[1]);
  port_stream >> http_port;
  int kv_port = http_port + 1;

  // Parse number of threads in IOService.
  int num_workers;
  std::istringstream thread_stream(argv[2]);
  thread_stream >> num_workers;

  // Setup the protocols. The HTTP server accepts requests to stop the
  // IOService machinery and requests for its stats.
  ServiceManager service(num_workers);
  HTTPService http_service(http_port, &service);
  KVService kv_service(kv_port, &service);

  // Loop until IOService is stopped via /quit request against the
  // HTTP Service.
  service.run();

  return 0;
}
