/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpServer.h"
#include "ServerOptions.h"
#include "TcpConnection.h"
#include "Utility.h"

namespace tcp {

  TcpServer::TcpServer(const ServerOptions& options, TaskControllerPtr taskController) :
  _options(options),
  _taskController(taskController),
  _numberConnections(options._expectedTcpConnections),
  _ioContext(1),
  _tcpPort(options._tcpPort),
  _endpoint(boost::asio::ip::address_v4::any(), _tcpPort),
  _acceptor(_ioContext) {}

TcpServer::~TcpServer() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
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
    // + 1 for 'this'
    _threadPool.start(_numberConnections + 1);
    accept();
    _threadPool.push(shared_from_this());
  }
  if (ec)
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << ' ' << ec.what() << " _tcpPort=" << _tcpPort << std::endl;
  return !ec;
}

void TcpServer::stop() {
  _stopped.store(true);
  boost::system::error_code ignore;
  _acceptor.close(ignore);
  _threadPool.stop();
  _ioContext.stop();
}

void TcpServer::run() noexcept {
  boost::system::error_code ec;
  _ioContext.run(ec);
  if (ec)
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ':' << ec.what() << std::endl;
}

void TcpServer::accept() {
  auto connection =
    std::make_shared<TcpConnection>(_options, _taskController, shared_from_this());
  _acceptor.async_accept(connection->socket(),
			 connection->endpoint(),
			 [connection, this](boost::system::error_code ec) {
			   handleAccept(connection, ec);
			 });
}

void TcpServer::handleAccept(TcpConnectionPtr connection,
			     const boost::system::error_code& ec) {
  if (ec)
    CERR << __FILE__ << ':' << __LINE__ << ' ' <<__func__ << ':' << ec.what() << std::endl;
  else {
    connection->start();
    accept();
  }
}

void TcpServer::pushToThreadPool(RunnablePtr connection){
  _threadPool.push(connection);
}

} // end of namespace tcp
