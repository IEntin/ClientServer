/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClient.h"
#include "ClientOptions.h"
#include "Header.h"
#include "SessionDetails.h"
#include "Tcp.h"
#include "TcpHeartbeatClient.h"
#include "Utility.h"

namespace tcp {

TcpClient::TcpClient(const ClientOptions& options) :
  Client(options),
  _socket(_ioContext) {
  auto [endpoint, error] =
    setSocket(_ioContext, _socket, _options._serverHost, _options._tcpPort);
  if (error)
    throw(std::runtime_error(error.what()));
  _endpoint = endpoint;
  boost::system::error_code ec;
  SESSIONTYPE type = SESSIONTYPE::SESSION;
  size_t result[[maybe_unused]] =
    boost::asio::write(_socket, boost::asio::buffer(&type, 1), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
    throw(std::runtime_error(ec.what()));
  }
  char buffer[HEADER_SIZE] = {};
  boost::asio::read(_socket, boost::asio::buffer(buffer, HEADER_SIZE), ec);
  if (ec)
    throw(std::runtime_error(std::string(std::strerror(errno))));
  HEADER header = decodeHeader(std::string_view(buffer, HEADER_SIZE));
  PROBLEMS problem = getProblem(header);
  switch (problem) {
  case PROBLEMS::NONE :
    CLOG << "NO PROBLEMS\n";
    break;
  case PROBLEMS::MAX_TCP_SESSIONS:
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << "\n\t!!!!!!!!!\n"
	 << "\tThe number of running tcp sessions is at thread pool capacity.\n"
	 << "\tIf you do not close the client, it will wait in the queue for\n"
	 << "\ta thread available after one of already running tcp clients\n"
	 << "\tis closed. At this point the client will resume the run.\n"
	 << "\tYou can also close the client and try again later.\n"
	 << "\tThe relevant setting is \"MaxTcpSessions\" in ServerOptions.json.\n"
	 << "\t!!!!!!!!!\n";
    break;
  case PROBLEMS::MAX_TOTAL_SESSIONS:
    // TBD
    break;
  default:
    break;
  }
  if (_options._tcpHeartbeatEnabled) {
    _tcpHeartbeatClient = std::make_shared<TcpHeartbeatClient>(_options);
    if (!_tcpHeartbeatClient->start())
      return;;
    _threadPool.push(_tcpHeartbeatClient);
  }
}

TcpClient::~TcpClient() {
  if (_tcpHeartbeatClient)
    _tcpHeartbeatClient->stop();
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '\n';
}

bool TcpClient::send(const std::vector<char>& msg) {
  boost::system::error_code ec;
  size_t result[[maybe_unused]] =
    boost::asio::write(_socket, boost::asio::buffer(msg), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
    return false;
  }
  return true;
}

bool TcpClient::receive() {
  boost::system::error_code ec;
  _socket.wait(boost::asio::ip::tcp::socket::wait_read, ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
    return false;
  }
  char buffer[HEADER_SIZE] = {};
  boost::asio::read(_socket, boost::asio::buffer(buffer, HEADER_SIZE), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
    return false;
  }
  auto [uncomprSize, comprSize, compressor, diagnostics, ephemeral, clientId, problem] =
    decodeHeader(std::string_view(buffer, HEADER_SIZE));
  if (problem != PROBLEMS::NONE)
    return false;
  if (!readReply(uncomprSize, comprSize, compressor == COMPRESSORS::LZ4))
    return false;
  return true;
}

bool TcpClient::run() {
  try {
    CloseSocket closeSocket(_socket);
    return Client::run();
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what() << '\n';
    return false;
  }
  return true;
}

bool TcpClient::readReply(size_t uncomprSize, size_t comprSize, bool bcompressed) {
  thread_local static std::vector<char> buffer;
  buffer.resize(comprSize);
  boost::system::error_code ec;
  size_t transferred[[maybe_unused]] =
    boost::asio::read(_socket, boost::asio::buffer(buffer, comprSize), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
    return false;
  }
  return printReply(buffer, uncomprSize, comprSize, bcompressed);
}

} // end of namespace tcp
