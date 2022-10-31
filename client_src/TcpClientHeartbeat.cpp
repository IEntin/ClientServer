/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClientHeartbeat.h"
#include "Client.h"
#include "ClientOptions.h"
#include "Tcp.h"
#include "Utility.h"

namespace tcp {

TcpClientHeartbeat::TcpClientHeartbeat(const ClientOptions& options, std::string_view clientId) :
  _options(options),
 _socket(_ioContext) {
  auto [endpoint, error] =
    setSocket(_ioContext, _socket, _options._serverHost, _options._tcpPort);
  if (error)
    throw(std::runtime_error(error.what()));
  std::vector<char> buffer(HEADER_SIZE + clientId.size());
  encodeHeader(buffer.data(),
	       HEADERTYPE::HEARTBEAT,
	       clientId.size(),
	       clientId.size(),
	       COMPRESSORS::NONE,
	       false);
  std::copy(clientId.data(), clientId.data() + clientId.size(), buffer.data() + HEADER_SIZE);
  boost::system::error_code ec;
  size_t bytes[[maybe_unused]] =
    boost::asio::write(_socket, boost::asio::buffer(buffer), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
    throw(std::runtime_error(ec.what()));
  }
}

TcpClientHeartbeat::~TcpClientHeartbeat() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool TcpClientHeartbeat::start() {
  return true;
}

void TcpClientHeartbeat::stop() {}

void TcpClientHeartbeat::run() noexcept {
  try {
    while (!Client::stopped()) {
      boost::system::error_code ec;
      boost::asio::read(_socket, boost::asio::buffer(_heartbeatBuffer, HEADER_SIZE), ec);
      if (ec) {
	(ec == boost::asio::error::eof ? CLOG : CERR)
	  << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
	break;
      }
      CLOG << '*' << std::flush;
      encodeHeader(_heartbeatBuffer, HEADERTYPE::HEARTBEAT, 0, 0, COMPRESSORS::NONE, false);
      size_t result[[maybe_unused]] =
	boost::asio::write(_socket, boost::asio::buffer(_heartbeatBuffer, HEADER_SIZE), ec);
      if (ec) {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
	break;
      }
    }
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what() << std::endl;
  }
  catch (...) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ": unexpected exception" << std::endl;
  }
}

} // end of namespace tcp
