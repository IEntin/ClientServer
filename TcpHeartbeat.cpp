/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpHeartbeat.h"
#include "ServerOptions.h"
#include "TaskController.h"
#include "Utility.h"

namespace tcp {

TcpHeartbeat::TcpHeartbeat(const ServerOptions& options) :
  _options(options),
  _ioContext(1),
  _socket(_ioContext),
  _timer(_ioContext) {
  boost::system::error_code ignore;
  _socket.set_option(boost::asio::socket_base::linger(false, 0), ignore);
  _socket.set_option(boost::asio::socket_base::reuse_address(true), ignore);
}

TcpHeartbeat::~TcpHeartbeat() {
  boost::system::error_code ignore;
  _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
  _socket.close(ignore);
  _timer.cancel(ignore);
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '\n';
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
  return !ec;
}

void TcpHeartbeat::run() noexcept {
  readToken();
  _ioContext.run();
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":io_context::run() exit\n";
}

void TcpHeartbeat::stop() {}

bool TcpHeartbeat::sendToken() {
  char buffer[1] = {};
  buffer[0] = '\n';
  std::string_view message(buffer, 1);
  write(message);
  return true;
}

void TcpHeartbeat::readToken() {
  asyncWait();
  if (_stopped)
    return;
  boost::system::error_code ignore;
  _timer.cancel(ignore);
  _buffer = '\0';
  auto self = shared_from_this();
  boost::asio::async_read(_socket,
			  boost::asio::buffer(&_buffer, 1),
			  [this, self] (const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
			    if (!ec)
			      sendToken();
			    else
			      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
			    boost::system::error_code ignore;
			    _timer.cancel(ignore);
			  });
}

void TcpHeartbeat::write(std::string_view reply) {
  auto self = shared_from_this();
  boost::asio::async_write(_socket,
			   boost::asio::buffer(reply.data(), reply.size()),
			   [this, self](boost::system::error_code ec, size_t transferred[[maybe_unused]]) {
			     if (ec)
			       CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
			     boost::system::error_code ignore;
			     _timer.cancel(ignore);
			     _socket.close(ignore);
			   });
}

void TcpHeartbeat::asyncWait() {
  boost::system::error_code ignore;
  _timer.expires_from_now(std::chrono::seconds(_options._tcpTimeout), ignore);
  auto self = shared_from_this();
  _timer.async_wait([this, self](const boost::system::error_code& err) {
		      if (err != boost::asio::error::operation_aborted) {
			CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":timeout.\n";
			boost::system::error_code ignore;
			_socket.close(ignore);
			_timer.expires_at(boost::asio::steady_timer::time_point::max(), ignore);
		      }
		    });
}

} // end of namespace tcp
