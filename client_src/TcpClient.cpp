/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClient.h"
#include "ClientOptions.h"
#include "SignalWatcher.h"
#include "Subtask.h"
#include "Tcp.h"

namespace tcp {

TcpClient::TcpClient() :
  _socket(_ioContext) {
  auto [endpoint, error] =
    Tcp::setSocket(_ioContext, _socket);
  if (error)
    throw(std::runtime_error(error.what()));
  HEADER header{ HEADERTYPE::CREATE_SESSION, 0, 0, COMPRESSORS::NONE, false, false, _status };
  auto ec = Tcp::sendMsg(_socket, header);
  if (ec)
    throw(std::runtime_error(ec.what()));
  if (!receiveStatus())
    throw std::runtime_error("TcpClient::receiveStatus failed");
  Info << _socket.local_endpoint() << ' ' << _socket.remote_endpoint() << '\n';
}

TcpClient::~TcpClient() {
  if (auto ptr = _signalWatcher.lock(); ptr)
    ptr->stop();
  Trace << '\n';
}

bool TcpClient::run() {
  start();
  return Client::run();
}

bool TcpClient::send(const Subtask& subtask) {
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
  auto ec = Tcp::readMsg(_socket, header, _clientId);
  assert(!isCompressed(header) && "expected uncompressed");
  if (ec) {
    throw(std::runtime_error(ec.what()));
    return false;
  }
  _status = extractStatus(header);
  switch (_status) {
  case STATUS::MAX_OBJECTS_OF_TYPE:
    displayMaxSessionsOfTypeWarn("tcp");
    createSignalWatcher();
    break;
  case STATUS::MAX_TOTAL_OBJECTS:
    displayMaxTotalSessionsWarn();
    createSignalWatcher();
    break;
  default:
    break;
  }
  return true;
}

void TcpClient::createSignalWatcher() {
  std::function<void()> func = [this]() {
    boost::system::error_code ec;
    _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    if (ec) {
      LogError << ec.what() << '\n';
    }
  };
  auto ptr = std::make_shared<SignalWatcher>(_signalFlag, func);
  _signalWatcher = ptr;
  _threadPoolClient.push(ptr);
}

} // end of namespace tcp
