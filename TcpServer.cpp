/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpServer.h"
#include "ServerOptions.h"
#include "SessionDetails.h"
#include "TaskController.h"
#include "TcpHeartbeat.h"
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
  _threadPool(_options._maxTcpSessions + 1),
  _heartbeatThreadPool(10) {
}

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
  if (ec)
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' 
	 << ec.what() << " tcpAcceptorPort=" << _options._tcpAcceptorPort << '\n';
  return !ec;
}

void TcpServer::stop() {
  _stopped.store(true);
  boost::system::error_code ignore;
  _acceptor.close(ignore);
  _heartbeatThreadPool.stop();
  _threadPool.stop();
  _ioContext.stop();
}

void TcpServer::run() {
  try {
    _ioContext.run();
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << '\n';
  }
}

void TcpServer::accept() {
  auto details = std::make_shared<SessionDetails>();
  _acceptor.async_accept(details->_socket,
			 details->_endpoint,
			 [details, this](boost::system::error_code ec) {
			   if (ec)
			     (ec == boost::asio::error::operation_aborted ? CLOG : CERR)
			       << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
			   else {
			     char type = '\0';
			     boost::asio::read(details->_socket, boost::asio::buffer(&type, 1), ec);
			     if (ec) {
			       CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << ec.what() << '\n';
			       return;
			     }
			     if (type == 's') {
			       RunnablePtr session = std::make_shared<TcpSession>(_options, details, shared_from_this());
			       session->start();
			       [[maybe_unused]] PROBLEMS problem = _threadPool.push(session);
			     }
			     else if (type == 'h') {
			       RunnablePtr heartbeat = std::make_shared<TcpHeartbeat>(_options, details, shared_from_this());
			       heartbeat->start();
			       [[maybe_unused]] PROBLEMS problem = _heartbeatThreadPool.push(heartbeat);
			     }
			     accept();
			   }
			 });
}

} // end of namespace tcp
