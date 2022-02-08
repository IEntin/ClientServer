/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpServer.h"
#include "TcpConnection.h"
#include <iostream>

namespace tcp {

TcpServer::TcpServer(unsigned expectedNumberConnections, unsigned port, unsigned timeout) :
  _ioContext(1),
  _tcpPort(port),
  _timeout(timeout),
  _endpoint(boost::asio::ip::tcp::v4(), _tcpPort),
  _acceptor(_ioContext, _endpoint),
  _connectionThreadPool(expectedNumberConnections) {
  accept();
  _thread = std::thread(&TcpServer::run, this);
}

TcpServer::~TcpServer(){
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
  boost::system::error_code ignore;
  _acceptor.close(ignore);
  _ioContext.stop();
  if (_thread.joinable()) {
    _thread.join();
    std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << " ... _thread joined ..." << std::endl;
  }
}

void TcpServer::stop() {
  _stopped.store(true);
  _connectionThreadPool.stop();
}

void TcpServer::run() noexcept {
  boost::system::error_code ec;
  _ioContext.run(ec);
  if (ec)
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ':' << ec.what() << std::endl;
}

void TcpServer::accept() {
  auto connection = std::make_shared<TcpConnection>(_timeout, this);
  _acceptor.async_accept(connection->socket(),
			 connection->endpoint(),
			 [connection, this](boost::system::error_code ec) {
			   handleAccept(connection, ec);
			 });
}

void TcpServer::handleAccept(std::shared_ptr<TcpConnection> connection,
			     const boost::system::error_code& ec) {
  if (ec)
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' <<__func__ << ':'
	      << ec.what() << std::endl;
  else {
    connection->start();
    accept();
  }
}

void TcpServer::pushConnection(std::shared_ptr<TcpConnection> connection){
  _connectionThreadPool.push(connection);
}

} // end of namespace tcp
