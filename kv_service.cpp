#include "kv_service.hpp"

namespace kv {

using base::makeCallableMany;

KVService::KVService(int port, ServiceManager* service_manager)
  : service_manager_(service_manager) {
}

KVService::~KVService() {
}

} // namespace kv
