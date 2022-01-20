/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpServer.h"
#include "ProgramOptions.h"

extern volatile std::atomic<bool> stopFlag;

namespace tcp {

boost::asio::io_context TcpServer::_ioContext;
std::string TcpServer::_tcpPort = ProgramOptions::get("TcpPort", std::string());
boost::asio::ip::tcp::endpoint TcpServer::_endpoint(boost::asio::ip::tcp::v4(), std::atoi(_tcpPort.c_str()));
boost::asio::ip::tcp::acceptor TcpServer::_acceptor(_ioContext, _endpoint);
std::thread TcpServer::_thread;
std::set<std::shared_ptr<TcpConnection>> TcpServer::_connections;

void TcpServer::accept() {
  auto connection = std::make_shared<TcpConnection>(_ioContext);
  _acceptor.async_accept(connection->socket(),
			 connection->remoteEndpoint(),
			 [connection](boost::system::error_code ec) {
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
    filterConnections();
    _connections.insert(connection);
    std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ":number connections=" << _connections.size() << std::endl;
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
    accept();
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
    if ((*it)->isStopped())
      it = _connections.erase(it);
    else
      ++it;
}

} // end of namespace tcp
