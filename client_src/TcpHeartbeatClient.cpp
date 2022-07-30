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
      heartbeat(_options);
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

bool TcpHeartbeatClient::start() {
  return true;
}

void TcpHeartbeatClient::stop() {
  _stop.store(true);
}

} // end of namespace tcp
