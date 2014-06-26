// Code copyright Alberto Lerner and Matthew Mancuso
// See git blame for details

#ifndef MCP_KV_SERVICE_HEADER
#define MCP_KV_SERVICE_HEADER

#include <string>

#include "http_response.hpp"
#include "lock.hpp"
#include "lock_free_skip_list.hpp"
#include "request_stats.hpp"
#include "service_manager.hpp"

namespace kv {

using base::AcceptCallback;
using base::Notification;
using base::RequestStats;
using base::ServiceManager;
using http::Response;
using lock_free::LockFreeSkipList;
using std::string;

class KVClientConnection;
typedef base::Callback<void, KVClientConnection*> ConnectCallback;
typedef base::Callback<void, Response*> ResponseCallback;

class KVService {
public:
  KVService(int port, ServiceManager* service_manager);
  ~KVService();

  // Asks the service manager to stop all the registered services.
  void stop();

  // Tries to connect to 'host:port' and issues 'cb' with the
  // resulting attempt. ConnectCallback takes an HTTPConnection as
  // parameter.
  void asyncConnect(const string& host, int port, ConnectCallback* cb);

  // Synchronous dual of asyncConnect. The ownership of
  // HTTPClientConnection is transfered to the caller.
  void connect(const string& host, int port, KVClientConnection** conn);

  // accessors

  LockFreeSkipList<int> kv_store;
  ServiceManager*       service_manager() { return service_manager_; }

private:
  ServiceManager* service_manager_;  // not owned here

  void acceptConnection(int client_fd);

  // Completion call used in synchronous connection.
  void connectInternal(Notification* n,
                       KVClientConnection** user_conn,
                       KVClientConnection* new_conn);

  // Non-copyable, non-assignable.
  KVService(const KVService&);
  KVService& operator=(const KVService&);
};

} // namespace kv

#endif //  MCP_KV_SERVICE_HEADER
