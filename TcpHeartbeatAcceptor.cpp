/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpHeartbeatAcceptor.h"
#include "ServerOptions.h"
#include "TaskController.h"
#include "TcpHeartbeatAcceptor.h"
#include "TcpHeartbeat.h"
#include "Utility.h"

namespace tcp {

TcpHeartbeatAcceptor::TcpHeartbeatAcceptor(const ServerOptions& options) :
  _options(options),
  _ioContext(1),
  _tcpHeartbeatPort(_options._tcpHeartbeatPort),
  _endpoint(boost::asio::ip::address_v4::any(), _tcpHeartbeatPort),
  _acceptor(_ioContext),
  _threadPool(2) {}

TcpHeartbeatAcceptor::~TcpHeartbeatAcceptor() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '\n';
}

bool TcpHeartbeatAcceptor::start() {
  boost::system::error_code ec;
  _acceptor.open(_endpoint.protocol(), ec);
  if (!ec)
    _acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
  if (!ec)
    _acceptor.set_option(boost::asio::socket_base::linger(false, 0), ec);
  if (!ec)
    _acceptor.bind(_endpoint, ec);
  if (!ec)
    _acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
  if (!ec) {
    accept();
    _threadPool.push(shared_from_this());
  }
  if (ec)
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' 
	 << ec.what() << " _tcpHeartbeatPort=" << _tcpHeartbeatPort << '\n';

  return !ec;
}

void TcpHeartbeatAcceptor::stop() {
  _stopped.store(true);
  boost::system::error_code ignore;
  _acceptor.close(ignore);
  _threadPool.stop();
  _ioContext.stop();
}

void TcpHeartbeatAcceptor::run() {
  boost::system::error_code ec;
  _ioContext.run(ec);
  if (ec)
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
}

void TcpHeartbeatAcceptor::accept() {
  auto heartbeat = std::make_shared<TcpHeartbeat>(_options);
  _acceptor.async_accept(heartbeat->socket(),
			 heartbeat->endpoint(),
			 [heartbeat, this](boost::system::error_code ec) {
			   handleAccept(heartbeat, ec);
			 });
}

void TcpHeartbeatAcceptor::handleAccept(RunnablePtr heartbeat, const boost::system::error_code& ec) {
  if (ec)
    (ec == boost::asio::error::operation_aborted ? CLOG : CERR)
      << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
  else {
    heartbeat->start();
    [[maybe_unused]] PROBLEMS problem = _threadPool.push(heartbeat);
    accept();
  }
}

} // end of namespace tcp
