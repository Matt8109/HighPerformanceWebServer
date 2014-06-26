// Code copyright Alberto Lerner and Matthew Mancuso
// See git blame for details

#ifndef MCP_HTTP_SERVICE_HEADER
#define MCP_HTTP_SERVICE_HEADER

#include <string>

#include "lock.hpp"
#include "file_cache.hpp"
#include "request_stats.hpp"
#include "service_manager.hpp"

namespace http {

using std::string;
using base::AcceptCallback;
using base::FileCache;
using base::Notification;
using base::RequestStats;
using base::ServiceManager;

class HTTPClientConnection;
class Response;
typedef base::Callback<void, HTTPClientConnection*> ConnectCallback;
typedef base::Callback<void, Response*> ResponseCallback;

// The HTTPService allows HTTP connections to be formed, both on the
// client and on the server sides, and links them to a given
// ServiceManager.
//
// There are some HTTP documents that perform special tasks.  The
// ServiceManager can be stopped by issuing a '/quit' HTTP GET
// request. The '/stat' documet would return a statistics page for the
// underlying ServiceManager. Any other request attempt would result
// in trying to read a file from disk with that document name.
class HTTPService {
public:
  // Starts a listening HTTP service at 'port'. A HTTP service
  // instance always has a server running.
  HTTPService(int port, ServiceManager* service_manager);

  // Stops the listening server. Assumes no AcceptCallbacks are
  // pending execution (ie, the service_manager is stopped).
  ~HTTPService();

  // Server Side

  // TODO
  //   + Register Request Handlers (see HTTPServerConnection::handleRequest)

  // Asks the service manager to stop all the registered services.
  void stop();

  // Client side

  // Tries to connect to 'host:port' and issues 'cb' with the
  // resulting attempt. ConnectCallback takes an HTTPConnection as
  // parameter.
  void asyncConnect(const string& host, int port, ConnectCallback* cb);

  // Synchronous dual of asyncConnect. The ownership of
  // HTTPClientConnection is transfered to the caller.
  void connect(const string& host, int port, HTTPClientConnection** conn);

  // accessors

  RequestStats* stats() { return &stats_; }
  ServiceManager* service_manager() { return service_manager_; }
  FileCache* file_cache() { return &file_cache_; }

private:
  ServiceManager* service_manager_;  // not owned here
  RequestStats    stats_;
  FileCache       file_cache_;

  // Starts the server-side of a new HTTP connection established on
  // socket 'client_fd'.
  void acceptConnection(int client_fd);

  // Completion call used in synchronous connection.
  void connectInternal(Notification* n,
                       HTTPClientConnection** user_conn,
                       HTTPClientConnection* new_conn);
};

}  // namespace http

#endif // MCP_HTTP_SERVICE_HEADER
