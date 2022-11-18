/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClientHeartbeat.h"
#include "ClientOptions.h"
#include "Tcp.h"
#include "Utility.h"

namespace tcp {

std::string TcpClientHeartbeat::_clientId;
std:: string TcpClientHeartbeat::_serverHost;
std::string TcpClientHeartbeat::_tcpPort;

TcpClientHeartbeat::TcpClientHeartbeat(const ClientOptions& options) :
  _options(options),
  _socket(_ioContext) {
  _serverHost = _options._serverHost;
  _tcpPort = _options._tcpPort;
  auto [endpoint, error] =
    setSocket(_ioContext, _socket, _serverHost, _tcpPort);
  if (error) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << error.what() << std::endl;
    return;
  }
  HEADER header{ HEADERTYPE::CREATE_HEARTBEAT, 0, 0, COMPRESSORS::NONE, false, STATUS::NONE };
  auto [success, ec] = sendMsg(_socket, header, _clientId);
  if (!success)
    throw std::runtime_error(ec.what());
  readStatus();
}

TcpClientHeartbeat::~TcpClientHeartbeat() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

unsigned TcpClientHeartbeat::getNumberObjects() const {
  return _objectCounter._numberObjects;
}

bool TcpClientHeartbeat::start() {
  _threadPool.push(shared_from_this());
  return true;
}

void TcpClientHeartbeat::stop() {
  closeHeartbeat();
  _threadPool.stop();
}

void TcpClientHeartbeat::run() noexcept {
  char buffer[HEADER_SIZE] = {};
  try {
    while (true) {
      boost::system::error_code ec;
      boost::asio::read(_socket, boost::asio::buffer(buffer, HEADER_SIZE), ec);
      if (ec) {
	(ec == boost::asio::error::eof ? CLOG : CERR)
	  << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
	break;
      }
      CLOG << '*' << std::flush;
      encodeHeader(buffer, HEADERTYPE::HEARTBEAT, 0, 0, COMPRESSORS::NONE, false);
      size_t result[[maybe_unused]] =
	boost::asio::write(_socket, boost::asio::buffer(buffer, HEADER_SIZE), ec);
      if (ec) {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
	break;
      }
    }
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what() << std::endl;
  }
  catch (...) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ": unexpected exception" << std::endl;
  }
}

void TcpClientHeartbeat::readStatus() {
  HEADER header;
  std::vector<char> payload;
  auto [success, ec] = readMsg(_socket, header, payload);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << ec.what() << std::endl;
    return;
  }
  _clientId.assign(payload.data(), payload.size());
  _status = getStatus(header);
  switch (_status) {
  case STATUS::NONE:
    break;
  default:
    break;
  }
}

bool TcpClientHeartbeat::closeHeartbeat() {
  boost::asio::io_context ioContext;
  boost::asio::ip::tcp::socket socket(ioContext);
  auto [endpoint, error] =
    setSocket(ioContext, socket, _serverHost, _tcpPort);
  if (error)
    return false;
  size_t size = _clientId.size();
  HEADER header{ HEADERTYPE::DESTROY_HEARTBEAT, size, size, COMPRESSORS::NONE, false, STATUS::NONE };
  return sendMsg(socket, header, _clientId).first;
}

} // end of namespace tcp
