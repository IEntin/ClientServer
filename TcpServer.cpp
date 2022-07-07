/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpServer.h"
#include "TaskController.h"
#include "ServerOptions.h"
#include "TcpConnection.h"
#include "Utility.h"

namespace tcp {

TcpServer::TcpServer(const ServerOptions& options) :
  Runnable(RunnablePtr(), TaskController::instance()),
  _options(options),
  _ioContext(1),
  _tcpPort(_options._tcpPort),
  _endpoint(boost::asio::ip::address_v4::any(), _tcpPort),
  _acceptor(_ioContext),
  // + 1 for 'this'
  _threadPool(_options._maxTcpConnections + 1) {}

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
	 << ec.what() << " _tcpPort=" << _tcpPort << '\n';
  return !ec;
}

void TcpServer::stop() {
  _stopped.store(true);
  boost::system::error_code ignore;
  _acceptor.close(ignore);
  _threadPool.stop();
  _ioContext.stop();
}

void TcpServer::run() {
  boost::system::error_code ec;
  _ioContext.run(ec);
  if (ec)
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
}

void TcpServer::accept() {
  auto connection =
    std::make_shared<TcpConnection>(_options, shared_from_this());
  _acceptor.async_accept(connection->socket(),
			 connection->endpoint(),
			 [connection, this](boost::system::error_code ec) {
			   handleAccept(connection, ec);
			 });
}

void TcpServer::handleAccept(TcpConnectionPtr connection, const boost::system::error_code& ec) {
  if (ec)
    (ec == boost::asio::error::operation_aborted ? CLOG : CERR)
      << __FILE__ << ':' << __LINE__ << ' ' <<__func__ << ':' << ec.what() << '\n';
  else {
    connection->start();
    _threadPool.push(connection);
    if (_typedConnections >= _options._maxTcpConnections)
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	   << "\nnumber running tcp connections=" << _typedConnections << " at thread pool capacity,\n"
	   << "tcp client will wait in the pool queue.\n"
	   << "close one of running tcp connections\n"
	   << "or increase \"MaxTcpConnections\" in ServerOptions.json.\n";
    accept();
  }
}

} // end of namespace tcp
