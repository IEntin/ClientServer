/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClient.h"
#include "ClientOptions.h"
#include "Logger.h"
#include "Metrics.h"
#include "Tcp.h"

namespace tcp {

TcpClient::TcpClient(const ClientOptions& options) :
  Client(options),
  _socket(_ioContext) {
  auto [endpoint, error] =
    setSocket(_ioContext, _socket, _options._serverHost, _options._tcpPort);
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
  Logger(LOG_LEVEL::TRACE) << CODELOCATION << std::endl;
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
    Error() << CODELOCATION << ':' << ec.what() << std::endl;
    return false;
  }
  if (stopped())
    return false;
  return true;
}

bool TcpClient::receive() {
  // receives interrupt and unblocks read on CtrlC in wait mode
  boost::system::error_code ec;
  _socket.wait(boost::asio::ip::tcp::socket::wait_read, ec);
  if (ec) {
    bool berror = true;
    switch (ec.value()) {
    case boost::asio::error::interrupted:
      berror = !stopped();
      break;
    default:
      berror = true;
      break;
    }
    Logger logger(berror ? LOG_LEVEL::ERROR : LOG_LEVEL::DEBUG, berror ? std::cerr : std::clog);
    logger << CODELOCATION << ':' << ec.what() << std::endl;
    return false;
  }
  ec.clear();
  char buffer[HEADER_SIZE] = {};
  size_t result[[maybe_unused]] =
    boost::asio::read(_socket, boost::asio::buffer(buffer, HEADER_SIZE), ec);
  if (ec) {
    bool berror = true;
    switch (ec.value()) {
    case boost::asio::error::interrupted:
    case boost::asio::error::connection_refused:
      berror = !stopped();
      break;
    default:
      berror = true;
      break;
    }
    Logger logger(berror ? LOG_LEVEL::ERROR : LOG_LEVEL::DEBUG, berror ? std::cerr : std::clog);
    logger << CODELOCATION << ':' << ec.what() << std::endl;
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
    Error() << CODELOCATION << ':' << ec.what() << std::endl;
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
  case STATUS::NONE:
    break;
  case STATUS::MAX_SPECIFIC_SESSIONS:
    Logger(LOG_LEVEL::WARN) << CODELOCATION
	 << "\n\t!!!!!!!!!\n"
	 << "\tThe number of running tcp sessions is at pool capacity.\n"
	 << "\tThis client will wait in the queue for available thread.\n"
	 << "\tAny already running tcp client must be closed.\n"
	 << "\tAt this point this client starts running.\n"
	 << "\tYou can also close this client and try again later,\n"
	 << "\tbut spot in the queue will be lost.\n"
	 << "\tThe setting is \"MaxTcpSessions\" in ServerOptions.json.\n"
	 << "\t!!!!!!!!!" << std::endl;
    break;
  case STATUS::MAX_TOTAL_SESSIONS:
    // TBD
    break;
  default:
    break;
  }
  return true;
}

} // end of namespace tcp
