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
  _timer(_ioContext) {
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
  _timer.cancel(ignore);
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '\n';
}

void TcpHeartbeatClient::run() {
  try {
    _ioContext.run();
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << '\n';
    _heartbeatFailed.store(true);
  }
}

bool TcpHeartbeatClient::heartbeat() {
  boost::system::error_code ec;
  char data = '\n';
  size_t transferred = boost::asio::write(_socket, boost::asio::buffer(&data, 1), ec);
  if (!ec)
    transferred += boost::asio::read(_socket, boost::asio::buffer(&data, 1), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
    _heartbeatFailed.store(true);
    return false;
  }
  bool success = transferred == 2 && data == '\n';
  _heartbeatFailed.store(!success);
  return success;
}

void TcpHeartbeatClient::asyncWait() {
  boost::system::error_code ec;
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
    return;
  }
  auto self(weak_from_this());
  _timer.async_wait([this, self](const boost::system::error_code& e) {
		      auto ptr = self.lock();
		      if (ptr) {
			if (e) {
			  CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what() << '\n';
			  return;
			}
			if (!heartbeat())
			  return;
		      }
		      boost::system::error_code ec;
		      _timer.expires_from_now(std::chrono::milliseconds(_options._heartbeatPeriod), ec);
		      asyncWait();
		    });
}

bool TcpHeartbeatClient::start() {
  boost::system::error_code ec;
  if (!heartbeat())
    return false;
  asyncWait();
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
    return false;
  }
  return true;
}

void TcpHeartbeatClient::stop() {
  _ioContext.stop();
  _socket.close();
}

} // end of namespace tcp
