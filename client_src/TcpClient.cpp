/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClient.h"
#include "ClientOptions.h"
#include "Header.h"
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
  _endpoint = endpoint;
  SESSIONTYPE type = SESSIONTYPE::SESSION;
  std::ostringstream os;
  os << _socket.local_endpoint() << std::flush;
  _clientId = os.str();
  os.str("");
  os << std::underlying_type_t<HEADERTYPE>(type) << '|' << _clientId << '|' << std::flush;
  std::string msg = os.str();
  msg.append(HSMSG_SIZE - msg.size(), '\0');
  boost::system::error_code ec;
  size_t result[[maybe_unused]] =
    boost::asio::write(_socket, boost::asio::buffer(msg), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
    throw(std::runtime_error(ec.what()));
  }
  char buffer[HEADER_SIZE] = {};
  boost::asio::read(_socket, boost::asio::buffer(buffer, HEADER_SIZE), ec);
  if (ec)
    throw(std::runtime_error(std::strerror(errno)));
  HEADER header = decodeHeader(std::string_view(buffer, HEADER_SIZE));
  PROBLEMS problem = getProblem(header);
  switch (problem) {
  case PROBLEMS::NONE :
    break;
  case PROBLEMS::MAX_TCP_SESSIONS:
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << "\n\t!!!!!!!!!\n"
	 << "\tThe number of running tcp sessions is at thread pool capacity.\n"
	 << "\tIf you do not close the client, it will wait in the queue for\n"
	 << "\tavailable thread (one of already running tcp clients must be closed).\n"
	 << "\tAt this point the client will resume run.\n"
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
  _heartbeat = std::make_shared<TcpClientHeartbeat>(_options, _clientId);
  _heartbeat->start();
}

TcpClient::~TcpClient() {
  boost::system::error_code ignore;
  _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
  _socket.close(ignore);
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
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
  char buffer[HEADER_SIZE] = {};
  boost::asio::read(_socket, boost::asio::buffer(buffer, HEADER_SIZE), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
    return false;
  }
  auto [headerType, uncomprSize, comprSize, compressor, diagnostics, ephemeral, problem] =
    decodeHeader(std::string_view(buffer, HEADER_SIZE));
  if (problem != PROBLEMS::NONE)
    return false;
  else
    return readReply(uncomprSize, comprSize, compressor == COMPRESSORS::LZ4);
}

bool TcpClient::run() {
  try {
    CloseSocket closeSocket(_socket);
    bool success = Client::run();
    return success;
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
