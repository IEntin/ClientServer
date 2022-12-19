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
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool TcpClient::run() {
  struct OnDestroy {
    OnDestroy(Client* client) : _client(client) {}
    ~OnDestroy() {
      _client->destroySession();
    }
    Client* _client = nullptr;
  } onDestroy(this);
  start();
  return Client::run();
}

void TcpClient::waitHandler(const boost::system::error_code& ec) {
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
  }
}

bool TcpClient::send(const std::vector<char>& msg) {
  _socket.async_wait(boost::asio::ip::tcp::socket::wait_write, waitHandler);
  boost::system::error_code ec;
  size_t result[[maybe_unused]] =
    boost::asio::write(_socket, boost::asio::buffer(msg), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
    return false;
  }
  if (_stopFlag.test())
    return false;
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
  ssize_t comprSize = extractCompressedSize(header);
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

bool TcpClient::receiveStatus() {
  HEADER header;
  auto [success, ec] = readMsg(_socket, header, _clientId);
  if (ec) {
    throw(std::runtime_error(ec.what()));
    return false;
  }
  _status = extractStatus(header);
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
	 << "\tYou can also close the client and try again later, but you will\n"
	 << "\tlose your spot in the queue starting from scratch.\n"
	 << "\tThe relevant setting is \"MaxTcpSessions\" in ServerOptions.json.\n"
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

bool TcpClient::destroySession() {
  try {
    boost::asio::ip::tcp::socket socket(_ioContext);
    auto [endpoint, error] =
      tcp::setSocket(_ioContext, socket, _options._serverHost, _options._tcpPort);
    if (error)
      return false;
    size_t size = _clientId.size();
    HEADER header{ HEADERTYPE::DESTROY_SESSION , size, size, COMPRESSORS::NONE, false, _status };
    return tcp::sendMsg(socket, header, _clientId).first;
  }
  catch (const std::exception& e) {
    CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << std::endl;
    return false;
  }
  return true;
}

} // end of namespace tcp
