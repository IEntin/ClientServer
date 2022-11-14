/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClient.h"
#include "ClientOptions.h"
#include "SessionDetails.h"
#include "Tcp.h"
#include "TcpClientHeartbeat.h"
#include "Utility.h"

namespace tcp {

TcpClient::TcpClient(const ClientOptions& options) :
  Client(options),
  _socket(_ioContext) {
  auto [endpoint, error] =
    setSocket(_ioContext, _socket, _options._serverHost, _options._tcpPort);
  if (error)
    throw(std::runtime_error(error.what()));
  HEADER header{ HEADERTYPE::CREATE_SESSION, 0, 0, COMPRESSORS::NONE, false, STATUS::NONE };
  auto [success, ec] = sendMsg(_socket, header);
  if (!success)
    throw(std::runtime_error(ec.what()));
  readStatus();
  auto heartbeat = std::make_shared<TcpClientHeartbeat>(_options, _clientId);
  _threadPoolTcpHeartbeat.push(heartbeat);
}

TcpClient::~TcpClient() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool TcpClient::send(const std::vector<char>& msg) {
  boost::system::error_code ec;
  size_t result[[maybe_unused]] =
    boost::asio::write(_socket, boost::asio::buffer(msg), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
    return false;
  }
  return true;
}

bool TcpClient::receive() {
  boost::system::error_code ec;
  char buffer[HEADER_SIZE] = {};
  size_t result[[maybe_unused]] =
    boost::asio::read(_socket, boost::asio::buffer(buffer, HEADER_SIZE), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
    return false;
  }
  _status.store(STATUS::NONE);
  HEADER header = decodeHeader(buffer);
  return readReply(header);
}

bool TcpClient::readReply(const HEADER& header) {
  thread_local static std::vector<char> buffer;
  ssize_t comprSize = getCompressedSize(header);
  buffer.resize(comprSize);
  boost::system::error_code ec;
  size_t transferred[[maybe_unused]] =
    boost::asio::read(_socket, boost::asio::buffer(buffer, comprSize), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
    return false;
  }
  return printReply(buffer, header);
}

void TcpClient::readStatus() {
  HEADER header;
  std::vector<char> payload;
  auto [success, ec] = readMsg(_socket, header, payload);
  if (ec)
    throw(std::runtime_error(ec.what()));
  _clientId.assign(payload.data(), payload.size());
  _status = getStatus(header);
  switch (_status) {
  case STATUS::NONE:
    break;
  case STATUS::MAX_SPECIFIC_SESSIONS:
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << "\n\t!!!!!!!!!\n"
	 << "\tThe number of running tcp sessions exceeds thread pool capacity.\n"
	 << "\tIf you do not close the client, it will wait in the queue for\n"
	 << "\tavailable thread (one of already running tcp clients must be closed).\n"
	 << "\tAt this point the client will resume run.\n"
	 << "\tYou can also close the client and try again later.\n"
	 << "\tThe relevant setting is \"MaxTcpSessions\" in ServerOptions.json.\n"
	 << "\t!!!!!!!!!" << std::endl;
    break;
  case STATUS::MAX_TOTAL_SESSIONS:
    // TBD
    break;
  default:
    break;
  }
}

} // end of namespace tcp
