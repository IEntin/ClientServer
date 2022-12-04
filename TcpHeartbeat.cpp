/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpHeartbeat.h"
#include "ConnectionDetails.h"
#include "ServerOptions.h"
#include "Tcp.h"
#include "Utility.h"

namespace tcp {

TcpHeartbeat::TcpHeartbeat(const ServerOptions& options,
			   ConnectionDetailsPtr details,
			   std::string_view clientId) :
  _options(options),
  _clientId(clientId),
  _details(details),
  _ioContext(_details->_ioContext),
  _strand(boost::asio::make_strand(_ioContext)),
  _socket(_details->_socket),
  _heartbeatTimer(_ioContext) {
  _heartbeatTimer.expires_from_now(std::chrono::milliseconds(std::numeric_limits<int>::max()));
  _socket.set_option(boost::asio::socket_base::reuse_address(true));
}

TcpHeartbeat::~TcpHeartbeat() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool TcpHeartbeat::start() {
  checkCapacity();
  size_t size = _clientId.size();
  HEADER header{ HEADERTYPE::CREATE_SESSION, size, size, COMPRESSORS::NONE, false, _status };
  return sendMsg(_socket, header, _clientId).first;
}

void TcpHeartbeat::run() noexcept {
  try {
    heartbeatWait();
    _ioContext.run();
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << std::endl;
  }
}

void TcpHeartbeat::stop() {
  _stopped.store(true);
  _ioContext.post([this]() {
    boost::system::error_code ec;
    _heartbeatTimer.cancel(ec);
    if (ec)
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << ec.what() << std::endl;
  });
}

void TcpHeartbeat::heartbeatWait() {
  _heartbeatTimer.expires_from_now(std::chrono::milliseconds(_options._heartbeatPeriod));
  auto weak = weak_from_this();
  _heartbeatTimer.async_wait(boost::asio::bind_executor(_strand,
    [this, weak](const boost::system::error_code& ec) {
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
    }));
}

void TcpHeartbeat::read() {
  auto weakPtr = weak_from_this();
  boost::asio::async_read(_socket,
    boost::asio::buffer(_heartbeatBuffer), boost::asio::bind_executor(_strand,
    [this, weakPtr] (const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
      auto self = weakPtr.lock();
      if (!self)
	return;
      if (ec) {
	(ec == boost::asio::error::eof ? CLOG : CERR)
	  << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
	_ioContext.stop();
	return;
      }
      HEADER header = decodeHeader(_heartbeatBuffer);
      if (!isOk(header)) {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ": header is invalid." << std::endl;
	return;
      }
      heartbeatWait();
    }));
}

void TcpHeartbeat::write() {
  encodeHeader(_heartbeatBuffer, HEADERTYPE::HEARTBEAT, 0, 0, COMPRESSORS::NONE, false);
  auto weakPtr = weak_from_this();
  boost::asio::async_write(_socket,
    boost::asio::buffer(_heartbeatBuffer), boost::asio::bind_executor(_strand,
    [this, weakPtr](const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
      auto self = weakPtr.lock();
      if (!self)
	return;
      if (ec) {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
	return;
      }
      read();
    }));
}

} // end of namespace tcp
