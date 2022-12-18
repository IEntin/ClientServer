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
  _timeoutTimer(_ioContext) {}

TcpClientHeartbeat::~TcpClientHeartbeat() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool TcpClientHeartbeat::start() {
  boost::asio::post(_ioContext, [this] { write(); });
  _threadPool.push(shared_from_this());
  return true;
}

void TcpClientHeartbeat::run() noexcept {
  try {
    _ioContext.run();
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << std::endl;
  }
}

void TcpClientHeartbeat::stop() {
  _threadPool.stop();
}

void TcpClientHeartbeat::heartbeatWait() {
  boost::system::error_code ec;
  _periodTimer.expires_from_now(std::chrono::milliseconds(_options._heartbeatPeriod), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
    return;
  }
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
  });
}

void TcpClientHeartbeat::timeoutWait() {
  auto weakPtr = weak_from_this();
  boost::system::error_code ec;
  _timeoutTimer.expires_from_now(std::chrono::milliseconds(_options._heartbeatTimeout), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
    return;
  }
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
	if (berror)
	  _status.store(STATUS::HEARTBEAT_PROBLEM);
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
	CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":timeout" << std::endl;
      CLOG << '*' << std::flush;
      heartbeatWait();
    });
}

void TcpClientHeartbeat::write() {
  timeoutWait();
  boost::asio::ip::tcp::socket socket(_ioContext);
  _socket = std::move(socket);
  auto [endpoint, error] =
    setSocket(_ioContext, _socket, _options._serverHost, _options._tcpPort);
  if (error) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << error.what() << std::endl;
    return;
  }
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
	_status.store(STATUS::HEARTBEAT_PROBLEM);
	return;
      }
      read();
    });
}

bool TcpClientHeartbeat::destroy() {
  struct CallStop {
    CallStop(RunnablePtr heartbeat) : _heartbeat(heartbeat) {}
    ~CallStop() { _heartbeat->stop(); }
    RunnablePtr _heartbeat;
  } callStop(shared_from_this());
  try {
    boost::asio::post(_ioContext, [this] {
      _periodTimer.cancel();
      _timeoutTimer.cancel();
    });
    return true;
  }
  catch (const boost::system::system_error& e) {
    (e.code() == boost::asio::error::connection_refused ? CLOG : CERR)
      << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << std::endl;
    return false;
  }
}

} // end of namespace tcp
