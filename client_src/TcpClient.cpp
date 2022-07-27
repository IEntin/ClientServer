/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClient.h"
#include "ClientOptions.h"
#include "Header.h"
#include "Tcp.h"
#include "Utility.h"

extern volatile std::sig_atomic_t stopSignal;

namespace tcp {

TcpClient::TcpClient(const ClientOptions& options) :
  Client(options),
  _socket(_ioContext) {
  auto [endpoint, error] =
    setSocket(_ioContext, _socket, _options._serverHost, _options._tcpAcceptorPort);
  if (error)
    throw(std::runtime_error(error.what()));
  _endpoint = endpoint;
  char buffer[HEADER_SIZE] = {};
  boost::system::error_code ec;
  boost::asio::read(_socket, boost::asio::buffer(buffer, HEADER_SIZE), ec);
  if (ec)
    throw(std::runtime_error(std::string(std::strerror(errno))));
  HEADER header = decodeHeader(std::string_view(buffer, HEADER_SIZE));
  _problem = getProblem(header);
  switch (_problem) {
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
}

TcpClient::~TcpClient() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '\n';
}

bool TcpClient::send(const std::vector<char>& msg) {
  heartbeat();
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
  // already running
  if (_running.test_and_set() || _problem == PROBLEMS::NONE) {
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
  }
  // waiting in queue
  else {
    std::size_t bytes_readable = 0;
    do {
      // client stopping
      if (stopSignal)
	break;
      // server stopped
     heartbeat();
      boost::asio::socket_base::bytes_readable command(true);
      _socket.io_control(command);
      bytes_readable = command.get();
      if (bytes_readable == 0)
	std::this_thread::sleep_for(std::chrono::seconds(1));
    } while (bytes_readable == 0);
  }
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

bool TcpClient::heartbeat() {
  boost::asio::io_context ioContext;
  boost::asio::ip::tcp::socket heartbeatSocket(ioContext);
  CloseSocket closeSocket(heartbeatSocket);
  auto [endpoint, error] =
    setSocket(_ioContext, heartbeatSocket, _options._serverHost, _options._tcpHeartbeatPort);
  if (error)
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << error.what() << '\n';
  boost::system::error_code ec;
  char data = '\n';
  size_t transferred = boost::asio::write(heartbeatSocket, boost::asio::buffer(&data, 1), ec);
  if (!ec) {
    data = '\0';
    transferred = boost::asio::read(heartbeatSocket, boost::asio::buffer(&data, 1), ec);
  }
  if (ec)
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
  return !ec;
}

} // end of namespace tcp
