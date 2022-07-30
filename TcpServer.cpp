/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpServer.h"
#include "ServerOptions.h"
#include "TaskController.h"
#include "TcpHeartbeatAcceptor.h"
#include "TcpSession.h"
#include "Utility.h"

namespace tcp {

TcpServer::TcpServer(const ServerOptions& options) :
  Runnable(RunnablePtr(), TaskController::instance()),
  _options(options),
  _ioContext(1),
  _endpoint(boost::asio::ip::address_v4::any(), _options._tcpAcceptorPort),
  _acceptor(_ioContext),
  // + 1 for 'this'
  _threadPool(_options._maxTcpSessions + 1) {}

TcpServer::~TcpServer() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '\n';
}

bool TcpServer::start() {
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
  _heartbeatAcceptor = std::make_shared<TcpHeartbeatAcceptor>(_options);
  _heartbeatAcceptor->start();
  if (ec)
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' 
	 << ec.what() << " tcpAcceptorPort=" << _options._tcpAcceptorPort << '\n';

  return !ec;
}

void TcpServer::stop() {
  _stopped.store(true);
  boost::system::error_code ignore;
  _acceptor.close(ignore);
  _threadPool.stop();
  _ioContext.stop();
  _heartbeatAcceptor->stop();
}

void TcpServer::run() {
  _ioContext.run();
}

void TcpServer::accept() {
  auto session = std::make_shared<TcpSession>(_options, shared_from_this());
  _acceptor.async_accept(session->socket(),
			 session->endpoint(),
			 [session, this](boost::system::error_code ec) {
			   if (ec)
			     (ec == boost::asio::error::operation_aborted ? CLOG : CERR)
			       << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
			   else {
			     session->start();
			     [[maybe_unused]] PROBLEMS problem = _threadPool.push(session);
			     accept();
			   }
			 });
}

} // end of namespace tcp
