/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClient.h"
#include "ClientOptions.h"
#include "Metrics.h"
#include "Tcp.h"
#include "Utility.h"

namespace tcp {

TcpClient::TcpClient(const ClientOptions& options) :
  Client(options),
  _socket(_ioContext) {
  auto [endpoint, error] =
    setSocket(_ioContext, _socket, _options);
  if (error)
    throw(std::runtime_error(error.what()));
  HEADER header{ HEADERTYPE::CREATE_SESSION, 0, 0, COMPRESSORS::NONE, false, _status };
  auto [success, ec] = sendMsg(_socket, header);
  if (!success)
    throw(std::runtime_error(ec.what()));
  if (!receiveStatus())
    throw std::runtime_error("TcpClient::receiveStatus failed");
}

TcpClient::~TcpClient() {
  Metrics::save();
  Trace << std::endl;
}

bool TcpClient::run() {
  start();
  return Client::run();
}

bool TcpClient::send(const std::vector<char>& msg) {
  boost::system::error_code ec;
  size_t result[[maybe_unused]] =
    boost::asio::write(_socket, boost::asio::buffer(msg), ec);
  if (ec) {
    LogError << ec.what() << std::endl;
    return false;
  }
  return true;
}

bool TcpClient::receive() {
  // receives interrupt and unblocks read on CtrlC in wait mode
  boost::system::error_code ec;
  _socket.wait(boost::asio::ip::tcp::socket::wait_read, ec);
  if (ec) {
    LogError << ec.what() << std::endl;
    return false;
  }
  ec.clear();
  char buffer[HEADER_SIZE] = {};
  size_t result[[maybe_unused]] =
    boost::asio::read(_socket, boost::asio::buffer(buffer, HEADER_SIZE), ec);
  if (ec) {
    LogError << ec.what() << std::endl;
    return false;
  }
  _status = STATUS::NONE;
  HEADER header = decodeHeader(buffer);
  return readReply(header);
}

bool TcpClient::readReply(const HEADER& header) {
  thread_local static std::vector<char> buffer;
  ssize_t comprSize = extractCompressedSize(header);
  buffer.resize(comprSize);
  boost::system::error_code ec;
  size_t transferred[[maybe_unused]] =
    boost::asio::read(_socket, boost::asio::buffer(buffer, comprSize), ec);
  if (ec) {
    LogError << ec.what() << std::endl;
    return false;
  }
  return printReply(buffer, header);
}

bool TcpClient::receiveStatus() {
  HEADER header;
  auto [success, ec] = readMsg(_socket, header, _clientId);
  assert(!isCompressed(header) && "expected uncompressed");
  if (ec) {
    throw(std::runtime_error(ec.what()));
    return false;
  }
  _status = extractStatus(header);
  switch (_status) {
  case STATUS::MAX_SPECIFIC_OBJECTS:
    utility::displayMaxSpecificSessionsWarn("tcp");
    break;
  case STATUS::MAX_TOTAL_OBJECTS:
    utility::displayMaxTotalSessionsWarn();
    break;
  default:
    break;
  }
  return true;
}

bool TcpClient::destroySession() {
  try {
    boost::asio::ip::tcp::socket socket(_ioContext);
    auto [endpoint, error] =
      tcp::setSocket(_ioContext, socket, _options);
    if (error)
      return false;
    size_t size = _clientId.size();
    HEADER header{ HEADERTYPE::DESTROY_SESSION , size, size, COMPRESSORS::NONE, false, _status };
    return tcp::sendMsg(socket, header, _clientId).first;
  }
  catch (const std::exception& e) {
    Warn << e.what() << std::endl;
    return false;
  }
  return true;
}

} // end of namespace tcp
