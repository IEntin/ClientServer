/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClient.h"
#include "ClientOptions.h"
#include "SignalWatcher.h"
#include "TaskBuilder.h"
#include "Tcp.h"
#include "Utility.h"

namespace tcp {

TcpClient::TcpClient(const ClientOptions& options) :
  Client(options),
  _socket(_ioContext) {
  static const std::vector<char> empty;
  auto [endpoint, error] =
    Tcp::setSocket(_ioContext, _socket, _options);
  if (error)
    throw(std::runtime_error(error.what()));
  HEADER header{ HEADERTYPE::CREATE_SESSION, 0, 0, COMPRESSORS::NONE, false, false, _status };
  auto [success, ec] = Tcp::sendMsg(_socket, header, empty);
  if (!success)
    throw(std::runtime_error(ec.what()));
  if (!receiveStatus())
    throw std::runtime_error("TcpClient::receiveStatus failed");
}

TcpClient::~TcpClient() {
  if (auto ptr = _signalWatcher.lock(); ptr)
    ptr->stop();
  Trace << std::endl;
}

bool TcpClient::run() {
  start();
  return Client::run();
}

bool TcpClient::send(const Subtask& subtask) {
  auto [success, ec] = Tcp::sendMsg(_socket, subtask._header, subtask._body);
  if (ec)
    LogError << ec.what() << std::endl;
  return success;
}

bool TcpClient::receive() {
  HEADER header;
  thread_local static std::vector<char> buffer;
  auto [success, ec] = Tcp::readMsg(_socket, header, buffer);
  if (ec) {
    LogError << ec.what() << std::endl;
    return false;
  }
  _status = STATUS::NONE;
  return printReply(header, buffer);
}

bool TcpClient::receiveStatus() {
  HEADER header;
  auto [success, ec] = Tcp::readMsg(_socket, header, _clientId);
  assert(!isCompressed(header) && "expected uncompressed");
  if (ec) {
    throw(std::runtime_error(ec.what()));
    return false;
  }
  _status = extractStatus(header);
  switch (_status) {
  case STATUS::MAX_OBJECTS_OF_TYPE:
    utility::displayMaxSessionsOfTypeWarn("tcp");
    createSignalWatcher();
    break;
  case STATUS::MAX_TOTAL_OBJECTS:
    utility::displayMaxTotalSessionsWarn();
    createSignalWatcher();
    break;
  default:
    break;
  }
  return true;
}

void TcpClient::createSignalWatcher() {
  boost::asio::ip::tcp::socket& socket(_socket);
  std::function<void()> func = [&socket]() {
    boost::system::error_code ec;
    socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    if (ec) {
      LogError << ec.what() << std::endl;
    }
  };
  auto ptr = std::make_shared<SignalWatcher>(_signalFlag, func);
  _signalWatcher = ptr;
  _threadPoolClient.push(ptr);
}

} // end of namespace tcp
