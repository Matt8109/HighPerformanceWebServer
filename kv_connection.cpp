#include "kv_connection.hpp"

namespace kv {

KVServerConnection::KVServerConnection(KVService* service, int client_fd)
  : Connection(service->service_manager()->io_manager(), client_fd),
    my_service_(service) {
}

bool KVServerConnection::readDone() {
  return false;
}

KVClientConnection::KVClientConnection(KVService* service)
  : Connection(service->service_manager()->io_manager()),
    my_service_(service) {
}

void KVClientConnection::connDone() {
}

bool KVClientConnection::readDone() {
  return false;
}

}  // namespace kv
