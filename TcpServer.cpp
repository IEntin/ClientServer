/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpServer.h"
#include "TcpConnection.h"
#include <iostream>

namespace tcp {

std::weak_ptr<TcpServer> TcpServer::_weakPtr;

TcpServer::TcpServer(unsigned expectedNumberConnections,
		     unsigned port,
		     unsigned timeout,
		     const std::pair<COMPRESSORS, bool>& compression) :
  _ioContext(1),
  _tcpPort(port),
  _timeout(timeout),
  _compression(compression),
  _endpoint(boost::asio::ip::tcp::v4(), _tcpPort),
  _acceptor(_ioContext, _endpoint),
  _connectionThreadPool(expectedNumberConnections) {
}

TcpServer::~TcpServer() {
  _ioContext.stop();
  if (_thread.joinable()) {
    _thread.join();
    std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << " ... _thread joined ..." << std::endl;
  }
}

void TcpServer::start(unsigned expectedNumberConnections,
		      unsigned port,
		      unsigned timeout,
		      const std::pair<COMPRESSORS, bool>& compression) {
  try {
    TcpServerPtr server = std::make_shared<TcpServer>(expectedNumberConnections,
						      port,
						      timeout,
						      compression);
    server->accept();
    server->_thread = std::thread(&TcpServer::run, server);
    _weakPtr = server;
  }
  catch (const std::exception& e) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << " exception caught:" << e.what() << std::endl;
  }
  catch (...) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << " exception caught:" << std::strerror(errno) << std::endl;
  }
}

void TcpServer::stop() {
  TcpServerPtr server = _weakPtr.lock();
  if (server) {
    server->_stopped.store(true);
    boost::system::error_code ignore;
    server->_acceptor.close(ignore);
    server->_connectionThreadPool.stop();
  }
}

void TcpServer::run() noexcept {
  boost::system::error_code ec;
  _ioContext.run(ec);
  if (ec)
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ':' << ec.what() << std::endl;
}

void TcpServer::accept() {
  auto connection = std::make_shared<TcpConnection>(_timeout, shared_from_this());
  _acceptor.async_accept(connection->socket(),
			 connection->endpoint(),
			 [connection, this](boost::system::error_code ec) {
			   handleAccept(connection, ec);
			 });
}

void TcpServer::handleAccept(TcpConnectionPtr connection,
			     const boost::system::error_code& ec) {
  if (ec)
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' <<__func__ << ':'
	      << ec.what() << std::endl;
  else {
    connection->start();
    accept();
  }
}

void TcpServer::pushConnection(TcpConnectionPtr connection){
  _connectionThreadPool.push(connection);
}

} // end of namespace tcp
