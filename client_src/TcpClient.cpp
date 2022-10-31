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
  std::ostringstream os;
  // socket.local_endpoint (ip address and ephemeral port)
  // is unique in a server-clients system no matter the
  // clients are on the same or different hosts and is
  // used as a client id
  os << _socket.local_endpoint() << std::flush;
  std::string clientId = os.str();
  std::vector<char> buffer(HEADER_SIZE + clientId.size());
  encodeHeader(buffer.data(),
	       HEADERTYPE::SESSION,
	       clientId.size(),
	       clientId.size(),
	       COMPRESSORS::NONE,
	       false);
  std::copy(clientId.data(), clientId.data() + clientId.size(), buffer.data() + HEADER_SIZE);
  boost::system::error_code ec;
  size_t bytes[[maybe_unused]] =
    boost::asio::write(_socket, boost::asio::buffer(buffer), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
    throw(std::runtime_error(ec.what()));
  }
  boost::asio::read(_socket, boost::asio::buffer(buffer, HEADER_SIZE), ec);
  if (ec)
    throw(std::runtime_error(std::strerror(errno)));
  HEADER header = decodeHeader(std::string_view(buffer.data(), HEADER_SIZE));
  _status = getProblem(header);
  switch (_status) {
  case STATUS::NONE:
    break;
  case STATUS::MAX_NUMBER_RUNNABLES:
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << "\n\t!!!!!!!!!\n"
	 << "\tThe number of running tcp sessions exceeds thread pool capacity.\n"
	 << "\tIf you do not close the client, it will wait in the queue for\n"
	 << "\tavailable thread (one of already running tcp clients must be closed).\n"
	 << "\tAt this point the client will resume run.\n"
	 << "\tYou can also close the client and try again later.\n"
	 << "\tThe relevant setting is \"MaxTcpSessions\" in ServerOptions.json.\n"
	 << "\t!!!!!!!!!\n";
    break;
  case STATUS::MAX_TOTAL_SESSIONS:
    // TBD
    break;
  default:
    break;
  }
  if (_options._tcpHeartbeatEnabled) {
    auto heartbeat = std::make_shared<TcpClientHeartbeat>(_options, clientId);
    _threadPool.push(heartbeat);
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
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
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
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
    return false;
  }
  HEADER header = decodeHeader(std::string_view(buffer, HEADER_SIZE));
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
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
    return false;
  }
  return printReply(buffer, header);
}

} // end of namespace tcp
