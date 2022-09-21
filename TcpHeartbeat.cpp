/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpHeartbeat.h"
#include "ServerOptions.h"
#include "SessionDetails.h"
#include "TcpAcceptor.h"
#include "TcpSession.h"
#include "Utility.h"

namespace tcp {

TcpHeartbeat::TcpHeartbeat(const ServerOptions& options, SessionDetailsPtr details, TcpAcceptorPtr parent) :
  _options(options),
  _details(details),
  _parent(parent),
  _ioContext(_details->_ioContext),
  _strand(boost::asio::make_strand(_ioContext)),
  _socket(_details->_socket),
  _heartbeatTimer(_ioContext),
  _heartbeatPeriod(_options._heartbeatPeriod) {
  _heartbeatTimer.expires_from_now(std::chrono::milliseconds(std::numeric_limits<int>::max()));
  _socket.set_option(boost::asio::socket_base::reuse_address(true));
}

TcpHeartbeat::~TcpHeartbeat() {
  if (_socket.is_open()) {
    boost::system::error_code ignore;
    _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
    _socket.close(ignore);
  }
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool TcpHeartbeat::start() {
  return true;
}

void TcpHeartbeat::run() noexcept {
  try {
    heartbeatWait();
    _ioContext.run();
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << '\n';
  }
}

unsigned TcpHeartbeat::getNumberObjects() const {
  return _objectCount._numberObjects;
}

void TcpHeartbeat::heartbeatWait() {
  if (_parent->stopped()) {
    _ioContext.stop();
    return;
  }
  _heartbeatTimer.expires_from_now(std::chrono::milliseconds(_heartbeatPeriod));
  _heartbeatTimer.async_wait(boost::asio::bind_executor(_strand,
    [this](const boost::system::error_code& ec) {
      if (_parent->stopped()) {
	_ioContext.stop();
	return;
      }
      if (ec) {
	if (ec != boost::asio::error::operation_aborted)
	  CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
	_ioContext.stop();
	return;
      }
      heartbeat();
      heartbeatWait();
    }));
}

void TcpHeartbeat::heartbeat() {
  if (_parent->stopped()) {
    _ioContext.stop();
    return;
  }
  encodeHeader(_heartbeatBuffer, HEADERTYPE::HEARTBEAT, 0, 0, COMPRESSORS::NONE, false, 0);
  boost::system::error_code ec;
  size_t result[[maybe_unused]] =
    boost::asio::write(_socket, boost::asio::buffer(_heartbeatBuffer, HEADER_SIZE), ec);
  if (ec) {
    if (ec != boost::asio::error::operation_aborted)
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
    _ioContext.stop();
    return;
  }
  char buffer[HEADER_SIZE] = {};
  boost::asio::read(_socket, boost::asio::buffer(buffer, HEADER_SIZE), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
    _ioContext.stop();
    return;
  }
  CLOG << '*' << std::flush;
}

} // end of namespace tcp
