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
  _timeoutTimer(_ioContext),
  _heartbeatBuffer(HEADER_SIZE) {}

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
  _periodTimer.expires_from_now(
    boost::posix_time::milliseconds(ClientOptions::_heartbeatPeriod), ec);
  if (ec) {
    LogError << ec.what() << '\n';
    return;
  }
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
  boost::system::error_code ec;
  _timeoutTimer.expires_from_now(
    boost::posix_time::milliseconds(ClientOptions::_heartbeatTimeout), ec);
  if (ec) {
    LogError << ec.what() << '\n';
    return;
  }
  _timeoutTimer.async_wait([this](const boost::system::error_code& ec) {
    auto self = weak_from_this().lock();
    if (!self)
      return;
    if (ec) {
      switch (ec.value()) {
      case boost::asio::error::eof:
      case boost::asio::error::connection_reset:
      case boost::asio::error::broken_pipe:
      case boost::asio::error::connection_refused:
	Info << ec.what() << '\n';
	break;
      case boost::asio::error::operation_aborted:
	break;
      default:
	LogError << ec.what() << '\n';
	break;
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
  _heartbeatBuffer.clear();
  boost::asio::async_read_until(_socket,
    boost::asio::dynamic_buffer(_heartbeatBuffer),
      utility::ENDOFMESSAGE,
      [this] (const boost::system::error_code& ec, std::size_t transferred) {
	if (_stopped)
	  return;
	auto self = weak_from_this().lock();
	if (!self)
	  return;
	std::size_t numberCanceled = _timeoutTimer.cancel();
	if (numberCanceled == 0) {
	  Warn << "timeout\n";
	  _status = STATUS::HEARTBEAT_TIMEOUT;
	}
	std::size_t ENDOFMESSAGESZ = utility::ENDOFMESSAGE.size();
	std::string_view receivedView(_heartbeatBuffer.data(), _heartbeatBuffer.size());
      if (transferred > ENDOFMESSAGESZ)
	if (receivedView.ends_with(utility::ENDOFMESSAGE))
	  _heartbeatBuffer.erase(_heartbeatBuffer.cend() - ENDOFMESSAGESZ);
      if (ec) {
	switch (ec.value()) {
	case boost::asio::error::eof:
	case boost::asio::error::connection_reset:
	case boost::asio::error::broken_pipe:
	case boost::asio::error::connection_refused:
	  Info << ec.what() << '\n';
	  break;
	default:
	  Warn << ec.what() << '\n';
	  break;
	}
	boost::asio::post(_ioContext, [this] {
	  stop();
	});
	_status = STATUS::HEARTBEAT_PROBLEM;
	return;
      }
      HEADER header;
      if (!deserialize(header, _heartbeatBuffer.data()))
	return;
      if (!isOk(header)) {
	LogError << "header is invalid." << '\n';
	return;
      }
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
  HEADER header{ HEADERTYPE::HEARTBEAT, 0, 0, COMPRESSORS::NONE, DIAGNOSTICS::NONE, _status, 0 };
  serialize(header, _heartbeatBuffer.data());
  std::array<boost::asio::const_buffer, 2> asioBuffers{ boost::asio::buffer(_heartbeatBuffer),
							boost::asio::buffer(utility::ENDOFMESSAGE) };
  boost::asio::async_write(_socket,
    asioBuffers,
    boost::asio::transfer_all(),
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
