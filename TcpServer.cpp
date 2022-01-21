/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpServer.h"
#include "ProgramOptions.h"

extern volatile std::atomic<bool> stopFlag;

namespace tcp {

boost::asio::io_context TcpServer::_ioContext;
unsigned TcpServer::_tcpPort = ProgramOptions::get("TcpPort", 0);
boost::asio::ip::tcp::endpoint TcpServer::_endpoint(boost::asio::ip::tcp::v4(), _tcpPort);
boost::asio::ip::tcp::acceptor TcpServer::_acceptor(_ioContext, _endpoint);
std::thread TcpServer::_thread;
std::set<std::shared_ptr<TcpConnection>> TcpServer::_connections;

void TcpServer::accept() {
  auto connection = std::make_shared<TcpConnection>(_ioContext);
  _acceptor.async_accept(connection->socket(),
			 connection->endpoint(),
			 [connection](boost::system::error_code ec) {
			   handleAccept(connection, ec);
			 });
}

void TcpServer::handleAccept(std::shared_ptr<TcpConnection> connection,
			     const boost::system::error_code& ec) {
  if (ec)
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' <<__func__ << ':'
	      << ec.what() << std::endl;
  else {
    filterConnections();
    _connections.insert(connection);
    std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ":connections#=" << _connections.size() << std::endl;
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
    _thread = std::move(std::thread(TcpServer::run));
  }
  catch (const std::exception& e) {
    std::cerr << "exception: " << e.what() << "\n";
  }
  return true;
}

void TcpServer::stopServer() {
  boost::system::error_code ignore;
  _acceptor.close(ignore);
  _ioContext.stop();
  if (_thread.joinable()) {
    _thread.join();
    std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << " ... _thread joined ..." << std::endl;
  }
  for (auto& connection : _connections)
    connection->stop();
  _connections.clear();
}

void TcpServer::filterConnections() {
  for (auto it = _connections.begin(); it != _connections.end();)
    if ((*it)->stopped())
      it = _connections.erase(it);
    else
      ++it;
}

} // end of namespace tcp
