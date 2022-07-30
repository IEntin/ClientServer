/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpHeartbeatClient.h"
#include "ClientOptions.h"
#include "Tcp.h"
#include "Utility.h"

namespace tcp {

std::atomic<bool> TcpHeartbeatClient::_serverDown;

TcpHeartbeatClient::TcpHeartbeatClient(const ClientOptions& options) :
  _options(options){}

TcpHeartbeatClient::~TcpHeartbeatClient() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '\n';
}

void TcpHeartbeatClient::run() {
  while (!_stop) {
    try {
      heartbeat();
      std::this_thread::sleep_for(std::chrono::milliseconds(_options._heartbeatPeriod));
      if (_stop)
	break;
    }
    catch(const std::exception& e) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << '\n';
      _serverDown.store(true);
      break;
    }
  }
}

bool TcpHeartbeatClient::heartbeat() {
  boost::asio::io_context ioContext;
  boost::asio::ip::tcp::socket socket(ioContext);
  CloseSocket closeSocket(socket);
  auto [endpoint, error] =
    setSocket(ioContext, socket, _options._serverHost, _options._tcpHeartbeatPort);
  if (error)
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << error.what() << '\n';
  boost::system::error_code ec;
  char data = '\n';
  size_t transferred = boost::asio::write(socket, boost::asio::buffer(&data, 1), ec);
  if (!ec) {
    data = '\0';
    transferred = boost::asio::read(socket, boost::asio::buffer(&data, 1), ec);
  }
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
    return false;
  }
  return transferred == 1 && data == '\n';
}

bool TcpHeartbeatClient::start() {
  return true;
}

void TcpHeartbeatClient::stop() {
  _stop.store(true);
}

} // end of namespace tcp
