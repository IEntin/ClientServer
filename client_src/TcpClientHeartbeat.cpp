/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClientHeartbeat.h"
#include "ConnectionDetails.h"
#include "ClientOptions.h"
#include "Tcp.h"
#include "Utility.h"

namespace tcp {

TcpClientHeartbeat::TcpClientHeartbeat(const ClientOptions& options) :
  _options(options),
  _socket(_ioContext),
  _periodTimer(_ioContext),
  _timeoutTimer(_ioContext) {
  _periodTimer.expires_from_now(std::chrono::milliseconds(std::numeric_limits<int>::max()));
  _timeoutTimer.expires_from_now(std::chrono::milliseconds(std::numeric_limits<int>::max()));
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

void TcpClientHeartbeat::run() noexcept {
  try {
    heartbeatWait();
    _ioContext.run();
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << std::endl;
  }
}

void TcpClientHeartbeat::stop() {
  _ioContext.post([this]() {
    boost::system::error_code ec;
    _periodTimer.cancel(ec);
    if (ec)
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << ec.what() << std::endl;
  });
  _threadPool.stop();
}

void TcpClientHeartbeat::heartbeatWait() {
  _periodTimer.expires_from_now(std::chrono::milliseconds(_options._heartbeatPeriod));
  auto weak = weak_from_this();
  _periodTimer.async_wait([this, weak](const boost::system::error_code& ec) {
    auto self = weak.lock();
    if (!self)
      return;
    if (_stopped)
      return;
    if (ec) {
      if (ec != boost::asio::error::operation_aborted)
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
      return;
    }
    write();
    CLOG << '*' << std::flush;
  });
}

void TcpClientHeartbeat::timeoutWait() {
  auto weakPtr = weak_from_this();
  _timeoutTimer.expires_from_now(std::chrono::milliseconds(_options._heartbeatTimeout));
  _timeoutTimer.async_wait([this, weakPtr](const boost::system::error_code& ec) {
      auto self = weakPtr.lock();
      if (!self)
	return;
      if (ec != boost::asio::error::operation_aborted) {
	if (ec)
	  CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
	else {
	  CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ": timeout" << std::endl;
	  _status.store(STATUS::HEARTBEAT_TIMEOUT);
	}
      }
    });
}

void TcpClientHeartbeat::read() {
  auto weakPtr = weak_from_this();
  boost::asio::async_read(_socket, boost::asio::buffer(_heartbeatBuffer),
    [this, weakPtr] (const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
      auto self = weakPtr.lock();
      if (!self)
	return;
      if (ec) {
	bool berror = false;
	switch (ec.value()) {
	case boost::asio::error::eof:
	case boost::asio::error::connection_reset:
	  break;
	default:
	  berror = true;
	  break;
	}
	(berror ? CERR : CLOG)
	  << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
	_ioContext.stop();
	return;
      }
      HEADER header = decodeHeader(_heartbeatBuffer);
      if (!isOk(header)) {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ": header is invalid." << std::endl;
	return;
      }
      std::size_t numberCanceled = _timeoutTimer.cancel();
      if (numberCanceled == 0)
	CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":there was a timeout" << std::endl;
      heartbeatWait();
    });
}

void TcpClientHeartbeat::write() {
  timeoutWait();
  encodeHeader(_heartbeatBuffer, HEADERTYPE::HEARTBEAT, 0, 0, COMPRESSORS::NONE, false);
  auto weakPtr = weak_from_this();
  boost::asio::async_write(_socket,
    boost::asio::buffer(_heartbeatBuffer),
    [this, weakPtr](const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
      auto self = weakPtr.lock();
      if (!self)
	return;
      if (ec) {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
	return;
      }
      read();
    });
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
