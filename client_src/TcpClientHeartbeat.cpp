/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClientHeartbeat.h"
#include "ClientOptions.h"
#include "Logger.h"
#include "Tcp.h"

namespace tcp {

TcpClientHeartbeat::TcpClientHeartbeat() :
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
  _periodTimer.expires_from_now(std::chrono::milliseconds(ClientOptions::_heartbeatPeriod));
  _periodTimer.async_wait([this](const boost::system::error_code& ec) {
    if (_stopped)
      return;
    auto self = weak_from_this().lock();
    if (!self)
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
  _timeoutTimer.expires_from_now(std::chrono::milliseconds(ClientOptions::_heartbeatTimeout));
  _timeoutTimer.async_wait([this](const boost::system::error_code& ec) {
    auto self = weak_from_this().lock();
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
  boost::asio::async_read(_socket, boost::asio::buffer(_heartbeatBuffer),
    [this] (const boost::system::error_code& ec, std::size_t transferred[[maybe_unused]]) {
      if (_stopped)
	return;
      auto self = weak_from_this().lock();
      if (!self)
	return;
      if (ec) {
	LogError << ec.what() << '\n';
	_status = STATUS::HEARTBEAT_PROBLEM;
	_ioContext.stop();
	return;
      }
      HEADER header;
      if (!deserialize(header, _heartbeatBuffer))
	return;
      if (!isOk(header)) {
	LogError << "header is invalid." << '\n';
	return;
      }
      std::size_t numberCanceled = _timeoutTimer.cancel();
      if (numberCanceled == 0)
	LogError << "timeout" << '\n';
      Logger logger(LOG_LEVEL::INFO, std::clog, false);
      logger << '*';
      heartbeatWait();
    });
}

void TcpClientHeartbeat::write() {
  timeoutWait();
  if (_socket.is_open())
    _socket.close();
  if (!Tcp::setSocket(_socket))
    return;
  // receives interrupt and unblocks read on CtrlC in wait mode
  boost::system::error_code ec;
  _socket.wait(boost::asio::ip::tcp::socket::wait_write, ec);
  if (ec) {
    LogError << ec.what() << '\n';
    return;
  }
  HEADER header{ HEADERTYPE::HEARTBEAT, 0, 0, COMPRESSORS::NONE, CRYPTO::NONE, DIAGNOSTICS::NONE, _status, 0 };
  serialize(header, _heartbeatBuffer);
  boost::asio::async_write(_socket,
    boost::asio::buffer(_heartbeatBuffer),
    [this](const boost::system::error_code& ec, std::size_t transferred[[maybe_unused]]) {
      auto self = weak_from_this().lock();
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
