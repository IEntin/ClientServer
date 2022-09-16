/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpHeartbeat.h"
#include "ServerOptions.h"
#include "TcpServer.h"
#include "TcpSession.h"
#include "Utility.h"

namespace tcp {

TcpHeartbeat::TcpHeartbeat(TcpSessionPtr session, TcpServerPtr parent) :
  _session(session),
  _parent(parent),
  _problem(session->_problem),
  _ioContext(session->_ioContext),
  _strand(session->_strand),
  _socket(session->_socket),
  _heartbeatTimer(_ioContext),
  _heartbeatPeriod(session->_heartbeatPeriod) {
  _heartbeatTimer.expires_from_now(std::chrono::milliseconds(std::numeric_limits<int>::max()));
}

TcpHeartbeat::~TcpHeartbeat() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool TcpHeartbeat::start() {
  _parent->pushHeartbeat(shared_from_this());
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

void TcpHeartbeat::heartbeatWait() {
  if (_stopped) {
    _ioContext.stop();
    return;
  }
  _heartbeatTimer.expires_from_now(std::chrono::milliseconds(_heartbeatPeriod));
  _heartbeatTimer.async_wait(boost::asio::bind_executor(_strand,
    [this](const boost::system::error_code& ec) {
      if (_stopped) {
	_ioContext.stop();
	return;
      }
      if (ec) {
	if (ec != boost::asio::error::operation_aborted)
	  CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
	_ioContext.stop();
	return;
      }
      if (_problem == PROBLEMS::MAX_TCP_SESSIONS)
	heartbeat();
      heartbeatWait();
    }));
}

void TcpHeartbeat::heartbeat() {
  if (_stopped) {
    _ioContext.stop();
    return;
  }
  CLOG << '.' << std::flush;
  encodeHeader(_heartbeatBuffer, HEADERTYPE::HEARTBEAT, 0, 0, COMPRESSORS::NONE, false, 0);
  _session->write(std::string_view(_heartbeatBuffer, HEADER_SIZE));
}

} // end of namespace tcp
