/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpHeartbeatClient.h"
#include "ClientOptions.h"
#include "SessionDetails.h"
#include "Tcp.h"
#include "Utility.h"

namespace tcp {

std::atomic<bool> TcpHeartbeatClient::_heartbeatFailed = false;

TcpHeartbeatClient::TcpHeartbeatClient(const ClientOptions& options) :
  _options(options),
  _socket(_ioContext),
  _timerPeriod(_ioContext),
  _timerTimeout(_ioContext) {
  auto [endpoint, error] =
    setSocket(_ioContext, _socket, _options._serverHost, _options._tcpPort);
  if (error)
    throw(std::runtime_error(error.what()));
  boost::system::error_code ec;
  SESSIONTYPE type = SESSIONTYPE::HEARTBEAT;
  size_t result[[maybe_unused]] =
    boost::asio::write(_socket, boost::asio::buffer(&type, 1), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
    throw(std::runtime_error(ec.what()));
  }
}

TcpHeartbeatClient::~TcpHeartbeatClient() {
  boost::system::error_code ignore;
  _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
  _socket.close(ignore);
  _timerPeriod.cancel(ignore);
  _timerTimeout.cancel(ignore);
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

void TcpHeartbeatClient::run() {
  try {
    asyncWaitPeriod();
    _ioContext.run();
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << '\n';
    _heartbeatFailed.store(true);
  }
}

void TcpHeartbeatClient::asyncWaitPeriod() {
  auto self(weak_from_this());
  _timerPeriod.async_wait([this, self](const boost::system::error_code& e) {
		      auto ptr = self.lock();
		      if (ptr) {
			if (e) {
			  CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what() << '\n';
			  return;
			}
			else {
			  asyncWaitTimeout();
			  // assume the worst
			  _heartbeatFailed.store(true);
			  sendToken();
			}
			boost::system::error_code ec;
			_timerPeriod.expires_from_now(std::chrono::milliseconds(_options._heartbeatPeriod), ec);
			asyncWaitPeriod();
		      }
		    });
}

void TcpHeartbeatClient::sendToken() {
  auto self = shared_from_this();
  boost::asio::async_write(_socket,
			   boost::asio::buffer(&_buffer, 1),
			  [this, self] (const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
			     if (!ec)
			       readToken();
			     else
			       CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
			   });
}

void TcpHeartbeatClient::readToken() {
  if (_stopped)
    return;
  _buffer = '\0';
  auto self = shared_from_this();
  boost::asio::async_read(_socket,
			  boost::asio::buffer(&_buffer, 1),
			  [this, self] (const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
			    boost::system::error_code error;
			    size_t canceled[[maybe_unused]] =_timerTimeout.cancel(error);
			    if (!ec) {
			      if (_buffer == '\n') {
				_heartbeatFailed.store(false);
				CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ": heartbeat success" << std::endl;
			      }
			    }
			    else
			      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
			  });
}

void TcpHeartbeatClient::asyncWaitTimeout() {
  boost::system::error_code ec;
  if (_timerTimeout.expires_from_now(boost::posix_time::milliseconds(_options._tcpHeartbeatTimeout), ec) > 0) {
    _timerTimeout.async_wait([] (const boost::system::error_code& error){
			       if (error != boost::asio::error::operation_aborted) {
				 CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << " timeout\n";
				 _heartbeatFailed.store(true);
			       }
			     });
  }
}

bool TcpHeartbeatClient::start() {
  return true;
}

void TcpHeartbeatClient::stop() {
  _ioContext.stop();
  boost::system::error_code ec;
  _timerPeriod.cancel(ec);
  _timerTimeout.cancel(ec);
  _socket.close(ec);
}

} // end of namespace tcp
