#ifndef MCP_KV_SERVICE_HEADER
#define MCP_KV_SERVICE_HEADER

#include "service_manager.hpp"

namespace kv {

using base::ServiceManager;

class KVService {
public:
  KVService(int port, ServiceManager* service_manager);
  ~KVService();

  // accessors

  ServiceManager* service_manager() { return service_manager_; }

private:
  ServiceManager* service_manager_;  // not owned here

  // Non-copyable, non-assignable.
  KVService(const KVService&);
  KVService& operator=(const KVService&);
};

} // namespace kv

#endif //  MCP_KV_SERVICE_HEADER
