/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClientHeartbeat.h"
#include "Header.h"
#include "ClientOptions.h"
#include "Tcp.h"
#include "Utility.h"

namespace tcp {

TcpClientHeartbeat::TcpClientHeartbeat(const ClientOptions& options, std::string_view clientId) :
  _options(options),
  _clientId(clientId),
  _socket(_ioContext) {
  auto [endpoint, error] =
    setSocket(_ioContext, _socket, _options._serverHost, _options._tcpPort);
  if (error)
    throw(std::runtime_error(error.what()));
  SESSIONTYPE type = SESSIONTYPE::HEARTBEAT;
  std::ostringstream os;
  os << std::underlying_type_t<HEADERTYPE>(type) << '|' << _clientId << '|' << std::flush;
  std::string msg = os.str();
  msg.append(HSMSG_SIZE - msg.size(), '\0');
  boost::system::error_code ec;
  size_t result[[maybe_unused]] =
    boost::asio::write(_socket, boost::asio::buffer(msg), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
    throw(std::runtime_error(ec.what()));
  }
}

TcpClientHeartbeat::~TcpClientHeartbeat() {
  if (_thread.joinable())
    _thread.join();
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool TcpClientHeartbeat::start() {
  _thread = std::jthread(&TcpClientHeartbeat::run, shared_from_this());
  return true;
}

void TcpClientHeartbeat::run() noexcept {
  try {
    while (true) {
      boost::system::error_code ec;
      boost::asio::read(_socket, boost::asio::buffer(_heartbeatBuffer, HEADER_SIZE), ec);
      if (ec) {
	(ec == boost::asio::error::eof ? CLOG : CERR)
	  << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
	break;
      }
      CLOG << '*' << std::flush;
      encodeHeader(_heartbeatBuffer, HEADERTYPE::HEARTBEAT, 0, 0, COMPRESSORS::NONE, false, 0);
      size_t result[[maybe_unused]] =
	boost::asio::write(_socket, boost::asio::buffer(_heartbeatBuffer, HEADER_SIZE), ec);
      if (ec) {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
	break;
      }
    }
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what() << '\n';
  }
  catch (...) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ": unexpected exception\n";
  }
  if (_thread.joinable())
    _thread.detach();
}

} // end of namespace tcp
