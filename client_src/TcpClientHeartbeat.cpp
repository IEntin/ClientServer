/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClientHeartbeat.h"
#include "ClientOptions.h"
#include "Tcp.h"
#include "Utility.h"

namespace tcp {

TcpClientHeartbeat::TcpClientHeartbeat(const ClientOptions& options) :
  _options(options),
  _socket(_ioContext) {
  auto [endpoint, error] =
    setSocket(_ioContext, _socket, _options._serverHost, _options._tcpPort);
  if (error) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << error.what() << std::endl;
    return;
  }
  if (!receiveStatus())
    throw std::runtime_error("TcpClientHeartbeat::receiveStatus failed");
}

TcpClientHeartbeat::~TcpClientHeartbeat() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool TcpClientHeartbeat::start() {
  _threadPool.push(shared_from_this());
  return true;
}

void TcpClientHeartbeat::stop() {
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

bool TcpClientHeartbeat::receiveStatus() {
  HEADER sendHeader{ HEADERTYPE::CREATE_HEARTBEAT, 0, 0, COMPRESSORS::NONE, false, STATUS::NONE };
  auto [sendSuccess, ecSend] = sendMsg(_socket, sendHeader);
  if (!sendSuccess)
    return false;
  HEADER rcvHeader;
  std::vector<char> payload;
  auto [success, ec] = readMsg(_socket, rcvHeader, payload);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << ec.what() << std::endl;
    return false;
  }
  _heartbeatId.assign(payload.data(), payload.size());
  _status = getStatus(rcvHeader);
  switch (_status) {
  case STATUS::NONE:
    break;
  default:
    break;
  }
  return true;
}

bool TcpClientHeartbeat::destroy() {
  struct CallStop {
    CallStop(RunnablePtr heartbeat) : _heartbeat(heartbeat) {}
    ~CallStop() { _heartbeat->stop(); }
    RunnablePtr _heartbeat;
  } callStop(shared_from_this());
  try {
    boost::asio::io_context ioContext;
    boost::asio::ip::tcp::socket socket(ioContext);
    auto [endpoint, error] =
      setSocket(ioContext, socket, _options._serverHost, _options._tcpPort);
    if (error)
      return false;
    size_t size = _heartbeatId.size();
    HEADER header{ HEADERTYPE::DESTROY_HEARTBEAT, size, size, COMPRESSORS::NONE, false, _status };
    auto [success, ec] = sendMsg(socket, header, _heartbeatId);
    return success;
  }
  catch (const boost::system::system_error& e) {
    (e.code() == boost::asio::error::connection_refused ? CLOG : CERR)
      << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << std::endl;
    return false;
  }
}

} // end of namespace tcp
