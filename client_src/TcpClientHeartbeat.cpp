/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClientHeartbeat.h"
#include "ClientOptions.h"
#include "Logger.h"
#include "Tcp.h"

namespace tcp {

TcpClientHeartbeat::TcpClientHeartbeat(const ClientOptions& options) :
  _options(options),
  _socket(_ioContext),
  _periodTimer(_ioContext),
  _timeoutTimer(_ioContext) {}

TcpClientHeartbeat::~TcpClientHeartbeat() {
  Trace << '\n';
}

bool TcpClientHeartbeat::start() {
  boost::asio::post(_ioContext, [this] { heartbeatWait(); });
  return true;
}

void TcpClientHeartbeat::run() {
  try {
    _ioContext.run();
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }
}

void TcpClientHeartbeat::stop() {
  try {
    _stopped = true;
    boost::asio::post(_ioContext, [this] {
      _periodTimer.cancel();
      _timeoutTimer.cancel();
    });
  }
  catch (const boost::system::system_error& e) {
    LogError << e.what() << '\n';
  }
}

void TcpClientHeartbeat::heartbeatWait() {
  boost::system::error_code ec;
  _periodTimer.expires_from_now(std::chrono::milliseconds(_options._heartbeatPeriod), ec);
  if (ec) {
    LogError << ec.what() << '\n';
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
	LogError << ec.what() << '\n';
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
    LogError << ec.what() << '\n';
    return;
  }
  _timeoutTimer.async_wait([this, weakPtr](const boost::system::error_code& ec) {
    auto self = weakPtr.lock();
    if (!self)
      return;
    if (ec != boost::asio::error::operation_aborted) {
      if (ec)
	LogError << ec.what() << '\n';
      else {
	LogError << "timeout" << '\n';
	_status = STATUS::HEARTBEAT_TIMEOUT;
      }
    }
  });
}

void TcpClientHeartbeat::read() {
  // receives interrupt and unblocks read on CtrlC in wait mode
  boost::system::error_code ec;
  _socket.wait(boost::asio::ip::tcp::socket::wait_read, ec);
  if (ec) {
    LogError << ec.what() << '\n';
    return;
  }
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
	  berror = !_stopped;
	  break;
	default:
	  berror = true;
	  break;
	}
	if (berror) {
	  LogError << ec.what() << '\n';
	  _status = STATUS::HEARTBEAT_PROBLEM;
	}
	else
	  Debug << ec.what() << '\n';
	_ioContext.stop();
	return;
      }
      HEADER header = decodeHeader(_heartbeatBuffer);
      if (!isOk(header)) {
	LogError << "header is invalid." << '\n';
	return;
      }
      std::size_t numberCanceled = _timeoutTimer.cancel();
      if (numberCanceled == 0)
	LogError << "timeout" << '\n';
      Logger(LOG_LEVEL::INFO, std::clog, false) << '*';
      // close socket early
      boost::system::error_code err;
      _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, err);
      if (err)
	LogError << err.what() << '\n';
      err.clear();
      _socket.close(err);
      if (err)
	LogError << err.what() << '\n';
      heartbeatWait();
    });
}

void TcpClientHeartbeat::write() {
  timeoutWait();
  boost::asio::ip::tcp::socket socket(_ioContext);
  _socket = std::move(socket);
  auto [endpoint, error] =
    Tcp::setSocket(_ioContext, _socket, _options);
  if (error) {
    LogError << error.what() << '\n';
    return;
  }
  // receives interrupt and unblocks read on CtrlC in wait mode
  boost::system::error_code ec;
  _socket.wait(boost::asio::ip::tcp::socket::wait_write, ec);
  if (ec) {
    LogError << ec.what() << '\n';
    return;
  }
  encodeHeader(_heartbeatBuffer, HEADERTYPE::HEARTBEAT, 0, 0, COMPRESSORS::NONE, false, false);
  auto weakPtr = weak_from_this();
  boost::asio::async_write(_socket,
    boost::asio::buffer(_heartbeatBuffer),
    [this, weakPtr](const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
      auto self = weakPtr.lock();
      if (!self)
	return;
      if (ec) {
	LogError << ec.what() << '\n';
	_status = STATUS::HEARTBEAT_PROBLEM;
	return;
      }
      read();
    });
}

} // end of namespace tcp
