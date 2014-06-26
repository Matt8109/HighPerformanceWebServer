// Code copyright Alberto Lerner and Matthew Mancuso
// See git blame for details

#include <fcntl.h>    // O_RDONLY
#include <stdio.h>    // perror
#include <stdlib.h>   // exit
#include <sstream>
#include <sys/stat.h> // fstat
#include <unistd.h>   // read, write, close

#include "buffer.hpp"
#include "http_connection.hpp"
#include "http_parser.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "request_stats.hpp"
#include "thread_pool_fast.hpp"
#include "ticks_clock.hpp"

namespace http {

using std::ostringstream;
using base::Buffer;
using base::FileCache;
using base::RequestStats;
using base::ThreadPoolFast;
using base::TicksClock;

HTTPServerConnection::HTTPServerConnection(HTTPService* service, int client_fd)
  : Connection(service->service_manager()->io_manager(), client_fd),
    my_service_(service),
    file_cache_(service->file_cache()) {
  startRead();
}

bool HTTPServerConnection::readDone() {
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

bool HTTPServerConnection::handleRequest(Request* request) {
  // This is a way to remotely shutdown the Server to which this
  // connection belongs.
  if (request_.address == "quit") {
    LOG(LogMessage::NORMAL) << "Server stop requested!";
    my_service_->stop();
    return false;
  }

  RequestStats* stats = my_service_->stats();
  if (request_.address == "stats") {
    uint32_t reqsLastSec;
    stats->getStats(TicksClock::getTicks(), &reqsLastSec);
    ostringstream stats_stream;
    stats_stream << reqsLastSec;
    string stats_string = stats_stream.str();

    m_write_.lock();

    out_.write("HTTP/1.1 200 OK\r\n");
    out_.write("Date: Wed, 28 Oct 2009 15:24:11 GMT\r\n");
    out_.write("Server: Lab02a\r\n");
    out_.write("Accept-Ranges: bytes\r\n");

    ostringstream os;
    os << "Content-Length: " << stats_string.size() << "\r\n";
    out_.write(os.str().c_str());
    out_.write("Content-Type: text/html\r\n");
    out_.write("\r\n");
    out_.write(stats_string.c_str());

    m_write_.unlock();

    startWrite();
    return true;
  }

  // If the request is for the root document, expand the name to
  // 'index.html'
  if (request_.address.empty()) {
    request_.address = "index.html";
  }

  // Grab from or load the file to the cache.
  Buffer* buf;
  int error;
  FileCache::CacheHandle h = file_cache_->pin(request_.address, &buf, & error);

  if (h != 0) {
    m_write_.lock();

    out_.write("HTTP/1.1 200 OK\r\n");
    out_.write("Date: Wed, 28 Oct 2009 15:24:11 GMT\r\n");
    out_.write("Server: Lab02a\r\n");
    out_.write("Accept-Ranges: bytes\r\n");

    ostringstream os;
    os << "Content-Length: " << buf->readSize() << "\r\n";
    out_.write(os.str().c_str());
    out_.write("Content-Type: text/html\r\n");
    out_.write("\r\n");

    // Copy from cache into the connection buffer.
    out_.copyFrom(buf);

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

    out_.write("HTTP/1.1 503 Bad Request\r\n");
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

  stats->finishedRequest(ThreadPoolFast::ME(), TicksClock::getTicks());

  startWrite();
  return true;
}

HTTPClientConnection::HTTPClientConnection(HTTPService* service)
  : Connection(service->service_manager()->io_manager()) { }

void HTTPClientConnection::connect(const string& host,
                                   int port,
                                   ConnectCallback* cb) {
  connect_cb_= cb;
  startConnect(host, port);
}

void HTTPClientConnection::connDone() {
  // If the connect process completed successfully, start reading from
  // the response stream.
  if (ok()) {
    startRead();
  }

  // Dispatch the user callback so they know they can start sending
  // requests now or handle the error.
  (*connect_cb_)(this);
}

bool HTTPClientConnection::readDone() {
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

bool HTTPClientConnection::handleResponse(Response* response) {
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

void HTTPClientConnection::asyncSend(Request* request, ResponseCallback* cb) {
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

void HTTPClientConnection::send(Request* request, Response** response) {
  Notification* n = new Notification;
  ResponseCallback* cb = makeCallableOnce(&HTTPClientConnection::receiveInternal,
                                          this,
                                          n,
                                          response);
  asyncSend(request, cb);
  n->wait();
  delete n;
}

void HTTPClientConnection::receiveInternal(Notification* n,
                                           Response** user_response,
                                           Response* new_response) {
  *user_response = new_response;
  n->notify();
}

}  // namespace http
