#include <fcntl.h>    // O_RDONLY
#include <stdio.h>    // perror
#include <stdlib.h>   // exit
#include <sstream>
#include <sys/stat.h> // fstat
#include <unistd.h>   // read, write, close

#include "buffer.hpp"
#include "kv_connection.hpp"
#include "http_parser.hpp"
#include "logging.hpp"
#include "request_stats.hpp"
#include "thread_pool_fast.hpp"
#include "ticks_clock.hpp"

namespace kv {

using base::Buffer;
using base::RequestStats;
using base::ThreadPoolFast;
using base::TicksClock;
using http::Parser;
using std::ostringstream;

KVServerConnection::KVServerConnection(KVService* service, int client_fd)
  : Connection(service->service_manager()->io_manager(), client_fd),
    my_service_(service) {
  startRead();
}

bool KVServerConnection::readDone() {
  int rc;

  while (true) {
    request_.clear();
    Buffer::Iterator it = in_.begin();
    rc = Parser::parseRequest(&it, &request_);
    if (rc < 0) {
      LOG(LogMessage::ERROR) << "Error parsing request";
      return false;

    } else if (rc > 0) {
      return true;

    } else /* rc == 0 */ {
      in_.consume(it.bytesRead());
      if (! handleRequest(&request_)) {
        return false;
      }
    }
  }
}

bool KVServerConnection::handleRequest(Request* request) {
  // This is a way to remotely shutdown the Server to which this
  // connection belongs.
  if (request_.address == "quit") {
    LOG(LogMessage::NORMAL) << "Server stop requested!";
    my_service_->stop();
    return false;
  }

  // If the request is for the root document, expand the name to
  // 'index.html'
  if (request_.address.empty()) {
    request_.address = "index.html";
  }

  // Grab from or load the file to the cache.
  Buffer* buf;
  int error;

  if (true) {
    m_write_.lock();

    out_.write("KV/1.1 200 OK\r\n");
    out_.write("Date: Wed, 28 Oct 2009 15:24:11 GMT\r\n");
    out_.write("Server: Lab02a\r\n");
    out_.write("Accept-Ranges: bytes\r\n");

    ostringstream os;
    os << "Content-Length: " << buf->readSize() << "\r\n";
    out_.write(os.str().c_str());
    out_.write("Content-Type: text/html\r\n");
    out_.write("\r\n");



    m_write_.unlock();

  } else {

    // TODO
    //
    // differentiate between a server internal error (ie too many
    // sockets open), which should return a 500 and a file not found,
    // which should return a 404.
    perror("Can't serve request");

    m_write_.lock();

    ostringstream html_stream;
    html_stream <<"<HTML>\r\n";
    html_stream <<"<HEAD><TITLE>400 Bad Request</TITLE></HEAD>\r\n";
    html_stream <<"<BODY>Bad Request</BODY>\r\n";
    html_stream <<"</HTML>\r\n";
    html_stream <<"\r\n";

    ostringstream os;
    os << "Content-Length: " << html_stream.str().size() << "\r\n";

    out_.write("KV/1.1 503 Bad Request\r\n");
    out_.write("Date: Wed, 28 Oct 2009 15:24:11 GMT\r\n");
    out_.write("Server: MyServer\r\n");
    out_.write("Connection: close\r\n");
    out_.write("Transfer-Encoding: chunked\r\n");
    out_.write(os.str().c_str());
    out_.write("Content-Type: text/html; charset=iso-8859-1\r\n");
    out_.write("\r\n");

    out_.write(html_stream.str().c_str());

    m_write_.unlock();

  }

  startWrite();
  return true;
}

KVClientConnection::KVClientConnection(KVService* service)
  : Connection(service->service_manager()->io_manager()) { }

void KVClientConnection::connect(const string& host,
                                   int port,
                                   ConnectCallback* cb) {
  connect_cb_= cb;
  startConnect(host, port);
}

void KVClientConnection::connDone() {
  // If the connect process completed successfully, start reading from
  // the response stream.
  if (ok()) {
    startRead();
  }

  // Dispatch the user callback so they know they can start sending
  // requests now or handle the error.
  (*connect_cb_)(this);
}

bool KVClientConnection::readDone() {
  int rc;

  while (true) {
    Buffer::Iterator it = in_.begin();
    Response* response = new Response;
    rc = Parser::parseResponse(&it, response);
    if (rc < 0) {
      delete response;
      LOG(LogMessage::ERROR) << "Error parsing response";
      return false;

    } else if (rc > 0) {
      delete response;
      return true;

    } else /* rc == 0 */ {
      in_.consume(it.bytesRead());
      if (! handleResponse(response)) { // response ownership xfer
        return false;
      }
      if (it.eob()) {
        return true;
      }
    }
  }
}

bool KVClientConnection::handleResponse(Response* response) {
  ResponseCallback* cb = NULL;
  m_response_.lock();
  if (!response_cbs_.empty()) {
    cb = response_cbs_.front();
    response_cbs_.pop();
  }
  m_response_.unlock();

  if (cb != NULL) {
    (*cb)(response);  // response ownership xfer
  }
  return true;
}

void KVClientConnection::asyncSend(Request* request, ResponseCallback* cb) {
  // Enqueue response callback before sending out the request,
  // otherwise the request may be sent out and come back before the
  // callback is present.
  m_response_.lock();
  response_cbs_.push(cb);

  m_write_.lock();
  request->toBuffer(&out_);
  m_write_.unlock();

  m_response_.unlock();

  startWrite();
}

void KVClientConnection::send(Request* request, Response** response) {
  Notification* n = new Notification;
  ResponseCallback* cb = makeCallableOnce(&KVClientConnection::receiveInternal,
                                          this,
                                          n,
                                          response);
  asyncSend(request, cb);
  n->wait();
  delete n;
}

void KVClientConnection::receiveInternal(Notification* n,
                                           Response** user_response,
                                           Response* new_response) {
  *user_response = new_response;
  n->notify();
}

}  // namespace kv
