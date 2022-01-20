/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpServer.h"
#include "ProgramOptions.h"

extern volatile std::atomic<bool> stopFlag;

namespace tcp {

boost::asio::io_context TcpServer::_ioContext;
std::thread TcpServer::_thread;
std::shared_ptr<TcpServer> TcpServer::_server;
std::set<std::shared_ptr<TcpConnection>> TcpServer::_connections;

TcpServer::TcpServer(const boost::asio::ip::tcp::endpoint& endpoint)
  : _acceptor(_ioContext, endpoint) {
  accept();
}

void TcpServer::accept() {
  filterConnections();
  auto connection = std::make_shared<TcpConnection>(_ioContext);
  _connections.insert(connection);
  _acceptor.async_accept(connection->socket(),
			 connection->remoteEndpoint(),
			 [this, connection](boost::system::error_code ec) {
			   handleAccept(connection, ec);
			 });
}

void TcpServer::handleAccept(std::shared_ptr<TcpConnection> connection,
			     const boost::system::error_code& ec) {
  if (ec) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' <<__func__ << ':'
	      << ec.what() << std::endl;
  }
  else {
    connection->start();
    accept();
  }
}

void TcpServer::run() noexcept {
  boost::system::error_code ec;
  while(!stopFlag) {
    _ioContext.restart();
    _ioContext.run(ec);
    if (ec)
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ':' << ec.what() << std::endl;
  }
}

bool TcpServer::startServer() {
  try {
    std::string tcpPort = ProgramOptions::get("TcpPort", std::string());
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), std::atoi(tcpPort.c_str()));
    _server = std::make_shared<TcpServer>(endpoint);
    std::thread tmp(TcpServer::run);
    _thread.swap(tmp);
  }
  catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }
  return true;
}

void  TcpServer::stopServer() {
  _ioContext.stop();
  if (_thread.joinable()) {
    _thread.join();
    std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << " ... _thread joined ..." << std::endl;
  }
}

void TcpServer::filterConnections() {
  for (auto it = _connections.begin(); it != _connections.end();)
    if ((*it)->isStopped()) {
      it = _connections.erase(it);
      std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ":number connections=" << _connections.size() << std::endl;;
    }
    else
      ++it;
}

} // end of namespace tcp
