/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClient.h"

#include "Subtask.h"
#include "Tcp.h"

namespace tcp {

TcpClient::TcpClient() : _socket(_ioContext) {
  auto error = Tcp::setSocket(_socket);
  if (error)
    throw(std::runtime_error(error.what()));
  HEADER header{ HEADERTYPE::CREATE_SESSION, 0, 0, COMPRESSORS::NONE, false, false, _status, 0 };
  auto ec = Tcp::sendMsg(_socket, header);
  if (ec)
    throw(std::runtime_error(ec.what()));
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
  auto ec = Tcp::sendMsg(_socket, subtask._header, subtask._body);
  if (ec)
    LogError << ec.what() << '\n';
  return !ec;
}

bool TcpClient::receive() {
  HEADER header;
  _response.clear();
  auto ec = Tcp::readMsg(_socket, header, _response);
  if (ec) {
    LogError << ec.what() << '\n';
    return false;
  }
  _status = STATUS::NONE;
  return printReply(header, _response);
}

bool TcpClient::receiveStatus() {
  HEADER header;
  std::string payload;
  auto ec = Tcp::readMsg(_socket, header, payload);
  if (!obtainKeyClientId(payload, header))
    return false;
  assert(!isCompressed(header) && "expected uncompressed");
  if (ec)
    throw(std::runtime_error(ec.what()));
  _status = extractStatus(header);
  switch (_status) {
  case STATUS::MAX_OBJECTS_OF_TYPE:
    displayMaxSessionsOfTypeWarn("tcp");
    break;
  case STATUS::MAX_TOTAL_OBJECTS:
    displayMaxTotalSessionsWarn();
    break;
  default:
    break;
  }
  std::size_t size = _Bstring.size();
  header =
    { HEADERTYPE::RECEIPT, size, size, COMPRESSORS::NONE, false, false, _status, size };
  ec = Tcp::sendMsg(_socket, header, _Bstring);
  if (ec)
    throw(std::runtime_error(ec.what()));
  return true;
}

void TcpClient::close() {
  boost::system::error_code ec;
  _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
  if (ec) {
    LogError << ec.what() << '\n';
  }
}

} // end of namespace tcp
