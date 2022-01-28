/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpServer.h"
#include <iostream>

namespace tcp {

std::shared_ptr<TcpServer> TcpServer::_instance;

TcpServer::TcpServer(unsigned port, unsigned timeout) :
  _tcpPort(port),
  _timeout(timeout),
  _endpoint(boost::asio::ip::tcp::v4(), _tcpPort),
  _acceptor(_ioContext, _endpoint) {
  accept();
}

void TcpServer::stop() {
  boost::system::error_code ignore;
  _acceptor.close(ignore);
  _ioContext.stop();
  if (_thread.joinable()) {
    _thread.join();
    std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << " ... _thread joined ..." << std::endl;
  }
}

void TcpServer::accept() {
  auto connection = std::make_shared<TcpConnection>(_ioContext, _timeout);
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

void TcpServer::run() noexcept {
  boost::system::error_code ec;
  _instance->_ioContext.run(ec);
  if (ec)
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ':' << ec.what() << std::endl;
}

bool TcpServer::startServer(unsigned port, unsigned timeout) {
  try {
    _instance = std::make_shared<TcpServer>(port, timeout);
    _instance->_thread = std::thread(TcpServer::run);
  }
  catch (const std::exception& e) {
    std::cerr << "exception: " << e.what() << "\n";
  }
  return true;
}

void TcpServer::stopServer() {
  _instance->stop();
}

} // end of namespace tcp
