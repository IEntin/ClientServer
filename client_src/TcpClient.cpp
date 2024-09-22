/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClient.h"

#include "IOUtility.h"
#include "Subtask.h"
#include "Tcp.h"

namespace tcp {

TcpClient::TcpClient() : _socket(_ioContext) {
  if (!Tcp::setSocket(_socket))
    throw std::runtime_error(utility::createErrorString());
  std::size_t size = _pubB.size();
  HEADER header =
    { HEADERTYPE::CREATE_SESSION, size, COMPRESSORS::NONE, CRYPTO::NONE, DIAGNOSTICS::NONE, _status, 0 };
  Tcp::sendMsg(_socket, header, _pubB);
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
  try {
    boost::system::error_code ec;
    _socket.wait(boost::asio::ip::tcp::socket::wait_write, ec);
    if (ec) {
      Warn << ec.what() << '\n';
      return false;
    }
    Tcp::sendMessage(_socket, subtask._data);
    return true;
  }
  catch (const std::exception& e) {
    Warn << e.what() << '\n';
    return false;
  }
}

bool TcpClient::receive() {
  try {
    _response.erase(0);
    boost::system::error_code ec;
    _socket.wait(boost::asio::ip::tcp::socket::wait_read, ec);
    if (ec) {
      Warn << ec.what() << '\n';
      return false;
    }
    if (!Tcp::readMessage(_socket, _response)) {
      return false;
    }
    _status = STATUS::NONE;
    return printReply();
  }
  catch (const std::exception& e) {
    Warn << e.what() << '\n';
    return false;
  }
}

bool TcpClient::receiveStatus() {
  std::string clientIdStr;
  CryptoPP::SecByteBlock pubAreceived;
  if (!Tcp::readMsg(_socket, _header, clientIdStr, pubAreceived))
    return false;
  ioutility::fromChars(clientIdStr, _clientId);
  if (!_dhB.Agree(_sharedB, _privB, pubAreceived))
    return false;
  assert(!isCompressed(_header) && "expected uncompressed");
  _status = extractStatus(_header);
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

} // end of namespace tcp
