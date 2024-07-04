/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClient.h"

#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include "Subtask.h"
#include "Tcp.h"

namespace tcp {

TcpClient::TcpClient() : _socket(_ioContext) {
  boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create, FIFO_NAMED_MUTEX);
  boost::interprocess::scoped_lock lock(mutex);
  Tcp::setSocket(_socket);
  std::size_t size = _pubBstring.size();
  HEADER header =
    { HEADERTYPE::CREATE_SESSION, size, size, COMPRESSORS::NONE, false, false, _status, 0 };
  Tcp::sendMsg(_socket, header, _pubBstring);
  if (!receiveStatus())
    throw std::runtime_error("TcpClient::receiveStatus failed");
  Info << _socket.local_endpoint() << ' ' << _socket.remote_endpoint() << '\n';
}

TcpClient::~TcpClient() {
  Trace << '\n';
}

void TcpClient::run() {
  start();
  Client::run();
}

bool TcpClient::send(Subtask& subtask) {
  Tcp::sendMsg(_socket, subtask._header, subtask._body);
  return true;
}

bool TcpClient::receive() {
  HEADER header;
  _response.clear();
  Tcp::readMsg(_socket, header, _response);
  _status = STATUS::NONE;
  return printReply(header, _response);
}

bool TcpClient::receiveStatus() {
  HEADER header;
  std::string payload;
  Tcp::readMsg(_socket, header, payload);
  if (!obtainKeyClientId(payload, header))
    return false;
  assert(!isCompressed(header) && "expected uncompressed");
  _status = extractStatus(header);
  switch (_status) {
  case STATUS::NONE:
    break;
  case STATUS::MAX_OBJECTS_OF_TYPE:
    displayMaxSessionsOfTypeWarn("tcp");
    break;
  case STATUS::MAX_TOTAL_OBJECTS:
    displayMaxTotalSessionsWarn();
    break;
  default:
    return false;
    break;
  }
  return true;
}

void TcpClient::close() {
  _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
}

} // end of namespace tcp
