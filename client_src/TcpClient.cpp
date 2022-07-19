/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClient.h"
#include "ClientOptions.h"
#include "Header.h"
#include "Tcp.h"
#include "Utility.h"

namespace tcp {

TcpClient::TcpClient(const ClientOptions& options) :
  Client(options),
  _socket(_ioContext) {
  if (!setSocket(_ioContext, _socket, _options._serverHost, _options._tcpAcceptorPort))
    throw(std::runtime_error(std::string(std::strerror(errno))));
  char buffer[HEADER_SIZE] = {};
  boost::system::error_code ec;
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
	 << "\n\t\t!!!!!!!\n"
	 << "\tThe number of running tcp sessions is at the thread pool capacity.\n"
	 << "\tYou do not have to close the client, it will wait in the pool queue\n"
	 << "\tfor the next available thread freed when some already running tcp client is closed.\n"
	 << "\tThen this client wil resume running.\n"
	 << "\tYou can also close this client and try again later.\n"
	 << "\tAnother option is to increase \"MaxTcpSessions\" in ServerOptions.json.\n"
	 << "\t\t!!!!!!!\n";
    break;
  case PROBLEMS::MAX_TOTAL_SESSIONS:
    // TBD
    break;
  default:
    break;
  }
}

TcpClient::~TcpClient() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool TcpClient::send(const std::vector<char>& msg) {
  boost::system::error_code ec;
  size_t result[[maybe_unused]] =
    boost::asio::write(_socket, boost::asio::buffer(msg), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << ':' << ec.what() << std::endl;
    return false;
  }
  return true;
}

bool TcpClient::receive() {
  char buffer[HEADER_SIZE] = {};
  boost::system::error_code ec;
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
