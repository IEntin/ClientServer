/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpHeartbeat.h"
#include "ConnectionDetails.h"
#include "Tcp.h"
#include "Utility.h"

namespace tcp {

TcpHeartbeat::TcpHeartbeat(ConnectionDetailsPtr details, std::string_view clientId) :
  _clientId(clientId),
  _details(details),
  _ioContext(_details->_ioContext),
  _socket(_details->_socket) {
  _socket.set_option(boost::asio::socket_base::reuse_address(true));
  boost::asio::post(_ioContext, [this] { read(); });
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
    _ioContext.run();
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << std::endl;
  }
}

void TcpHeartbeat::stop() {
  _stopped.store(true);
  _ioContext.stop();
}

void TcpHeartbeat::read() {
  auto weakPtr = weak_from_this();
  boost::asio::async_read(_socket,
    boost::asio::buffer(_heartbeatBuffer),
    [this, weakPtr] (const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
      auto self = weakPtr.lock();
      if (!self)
	return;
      if (ec) {
	bool berror = false;
	switch (ec.value()) {
	case boost::asio::error::eof:
	case  boost::asio::error::operation_aborted:
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
      write();
    });
}

void TcpHeartbeat::write() {
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
      CLOG << '*' << std::flush;
      read();
    });
}

} // end of namespace tcp
