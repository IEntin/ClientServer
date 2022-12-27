/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClientHeartbeat.h"
#include "Client.h"
#include "ClientOptions.h"
#include "ConnectionDetails.h"
#include "Tcp.h"
#include "Utility.h"

namespace tcp {

  TcpClientHeartbeat::TcpClientHeartbeat(const ClientOptions& options) :
  _options(options),
  _socket(_ioContext),
  _periodTimer(_ioContext),
  _timeoutTimer(_ioContext) {}

TcpClientHeartbeat::~TcpClientHeartbeat() {
  Logger(LOG_LEVEL::TRACE) << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
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
    Logger() << __FILE__ << ':' << __LINE__ << ' ' << __func__
	     << ' ' << e.what() << std::endl;
  }
}

void TcpClientHeartbeat::stop() {
  try {
    boost::asio::post(_ioContext, [this] {
      _periodTimer.cancel();
      _timeoutTimer.cancel();
    });
  }
  catch (const boost::system::system_error& e) {
    Logger() << __FILE__ << ':' << __LINE__ << ' ' << __func__
	     << ' ' << e.what() << std::endl;
  }
  _threadPool.stop();
}

void TcpClientHeartbeat::heartbeatWait() {
  boost::system::error_code ec;
  _periodTimer.expires_from_now(std::chrono::milliseconds(_options._heartbeatPeriod), ec);
  if (ec) {
    Logger() << __FILE__ << ':' << __LINE__ << ' ' << __func__
	     << ':' << ec.what() << std::endl;
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
	Logger() << __FILE__ << ':' << __LINE__ << ' ' << __func__
		 << ':' << ec.what() << std::endl;
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
    Logger() << __FILE__ << ':' << __LINE__ << ' ' << __func__
	     << ':' << ec.what() << std::endl;
    return;
  }
  _timeoutTimer.async_wait([this, weakPtr](const boost::system::error_code& ec) {
    auto self = weakPtr.lock();
    if (!self)
      return;
    if (ec != boost::asio::error::operation_aborted) {
      if (ec)
	Logger() << __FILE__ << ':' << __LINE__ << ' ' << __func__
		 << ':' << ec.what() << std::endl;
      else {
	Logger() << __FILE__ << ':' << __LINE__ << ' ' << __func__
		 << ": timeout" << std::endl;
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
	  if (Client::_stopFlag.test())
	    berror = false;
	  else
	    berror = true;
	  break;
	default:
	  berror = true;
	  break;
	}
	Logger logger(berror ? LOG_LEVEL::ERROR : LOG_LEVEL::DEBUG, berror ? std::cerr : std::clog);
	logger << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
	if (berror)
	  _status.store(STATUS::HEARTBEAT_PROBLEM);
	_ioContext.stop();
	return;
      }
      HEADER header = decodeHeader(_heartbeatBuffer);
      if (!isOk(header)) {
	Logger() << __FILE__ << ':' << __LINE__ << ' ' << __func__
		 << ": header is invalid." << std::endl;
	return;
      }
      std::size_t numberCanceled = _timeoutTimer.cancel();
      if (numberCanceled == 0)
	Logger(LOG_LEVEL::ERROR) << __FILE__ << ':' << __LINE__ << ' ' << __func__
          << ":timeout" << std::endl;
      Logger(LOG_LEVEL::INFO, std::clog, false) << "*" << std::flush;
      // close socket early
      boost::system::error_code err;
      _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, err);
      if (err)
	Logger() << __FILE__ << ':' << __LINE__ << ' ' << __func__
		 << ' ' << err.what() << std::endl;
      err.clear();
      _socket.close(err);
      if (err)
	Logger() << __FILE__ << ':' << __LINE__ << ' ' << __func__
		 << ' ' << err.what() << std::endl;
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
    Logger() << __FILE__ << ':' << __LINE__ << ' ' << __func__
	     << ' ' << error.what() << std::endl;
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
	Logger() << __FILE__ << ':' << __LINE__ << ' ' << __func__
		 << ':' << ec.what() << std::endl;
	_status.store(STATUS::HEARTBEAT_PROBLEM);
	return;
      }
      read();
    });
}

} // end of namespace tcp
