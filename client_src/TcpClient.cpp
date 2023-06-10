/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClient.h"
#include "ClientOptions.h"
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
  return printReply(std::string_view(buffer.data(), buffer.size()), header);
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
    break;
  case STATUS::MAX_TOTAL_OBJECTS:
    utility::displayMaxTotalSessionsWarn();
    break;
  default:
    break;
  }
  return true;
}

} // end of namespace tcp
