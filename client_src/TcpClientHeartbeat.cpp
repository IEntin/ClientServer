/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClientHeartbeat.h"
#include "Client.h"
#include "ClientOptions.h"
#include "Tcp.h"
#include "Utility.h"

namespace tcp {

TcpClientHeartbeat::TcpClientHeartbeat(const ClientOptions& options, std::string_view sessionClientId) :
  _options(options),
  _sessionClientId(sessionClientId),
 _socket(_ioContext) {
  auto [endpoint, error] =
    setSocket(_ioContext, _socket, _options._serverHost, _options._tcpPort);
  if (error)
    throw(std::runtime_error(error.what()));
  HEADER header{ HEADERTYPE::CREATE_HEARTBEAT, 0, 0, COMPRESSORS::NONE, false, STATUS::NONE };
  auto [success, ec] = sendMsg(_socket, header, _clientId);
  if (!success)
    throw(std::runtime_error(ec.what()));
  readStatus();
}

TcpClientHeartbeat::~TcpClientHeartbeat() {
  closeSession();
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

unsigned TcpClientHeartbeat::getNumberObjects() const {
  return _objectCounter._numberObjects;
}

bool TcpClientHeartbeat::start() {
  return true;
}

void TcpClientHeartbeat::stop() {}

void TcpClientHeartbeat::run() noexcept {
  char buffer[HEADER_SIZE] = {};
  try {
    while (!Client::stopped()) {
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
    closeHeartbeat();
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
  if (ec)
    throw(std::runtime_error(ec.what()));
  _clientId.assign(payload.data(), payload.size());
  _status = getProblem(header);
  switch (_status) {
  case STATUS::NONE:
    break;
  default:
    break;
  }
}

bool TcpClientHeartbeat::closeHeartbeat() {
  boost::asio::ip::tcp::socket socket(_ioContext);
  auto [endpoint, error] =
    setSocket(_ioContext, socket, _options._serverHost, _options._tcpPort);
  if (error)
    return false;
  size_t size = _clientId.size();
  HEADER header{ HEADERTYPE::DESTROY_HEARTBEAT, size, size, COMPRESSORS::NONE, false, _status };
  return sendMsg(socket, header, _clientId).first;
}

bool TcpClientHeartbeat::closeSession() {
  boost::asio::ip::tcp::socket socket(_ioContext);
  auto [endpoint, error] =
    setSocket(_ioContext, socket, _options._serverHost, _options._tcpPort);
  if (error)
    return false;
  size_t size = _sessionClientId.size();
  HEADER header{ HEADERTYPE::DESTROY_SESSION , size, size, COMPRESSORS::NONE, false, _status };
  return sendMsg(socket, header, _sessionClientId).first;
}

} // end of namespace tcp
