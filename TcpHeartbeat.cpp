/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpHeartbeat.h"
#include "ServerOptions.h"
#include "SessionDetails.h"
#include "TaskController.h"
#include "Utility.h"

namespace tcp {

TcpHeartbeat::TcpHeartbeat(const ServerOptions& options, SessionDetailsPtr details, RunnablePtr parent) :
  Runnable(parent, TaskController::instance()),
  _options(options),
  _details(details),
  _ioContext(details->_ioContext),
  _socket(details->_socket),
  _timer(_ioContext) {}

TcpHeartbeat::~TcpHeartbeat() {
  boost::system::error_code ignore;
  _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
  _socket.close(ignore);
  _timer.cancel(ignore);
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool TcpHeartbeat::start() {
  boost::system::error_code ec;
  _socket.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
  if (!ec)
    _socket.set_option(boost::asio::socket_base::linger(false, 0), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
    return false;
  }
  readToken();
  return !ec;
}

void TcpHeartbeat::run() noexcept {
  try{
    _ioContext.run();
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << '\n';
  }
}

void TcpHeartbeat::stop() {}

void TcpHeartbeat::sendToken() {
  auto self = shared_from_this();
  boost::asio::async_write(_socket,
			   boost::asio::buffer(&_buffer, 1),
			  [this, self] (const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
			     if (ec)
			       CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
			     size_t canceled =
			       _timer.expires_from_now(std::chrono::milliseconds(std::numeric_limits<int>::max()));
			     if (canceled == 0)
			       CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ": timeout\n";
			     readToken();
			   });
}

void TcpHeartbeat::readToken() {
  auto self = shared_from_this();
  boost::asio::async_read(_socket,
			  boost::asio::buffer(&_buffer, 1),
			  [this, self] (const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
			    if (_stopped)
			      return;
			    if (ec) {
			      (ec == boost::asio::error::eof ? CLOG : CERR)
				<< __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
			      return;
			    }
			    asyncWait();
			    sendToken();
			  });
}

void TcpHeartbeat::asyncWait() {
  _timer.expires_from_now(std::chrono::milliseconds(_options._tcpTimeout));
  auto self = shared_from_this();
  _timer.async_wait([self](const boost::system::error_code& err) {
		      if (err != boost::asio::error::operation_aborted)
			CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":timeout.\n";
		    });
}

} // end of namespace tcp
