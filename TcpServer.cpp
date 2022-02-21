/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpServer.h"
#include "TcpConnection.h"
#include <iostream>

namespace tcp {

TcpServerPtr TcpServer::_instance;

TcpServer::TcpServer(unsigned expectedNumberConnections,
		     unsigned port,
		     unsigned timeout,
		     const CompressionType& compression) :
  _numberThreads(expectedNumberConnections),
  _ioContext(1),
  _tcpPort(port),
  _timeout(timeout),
  _compression(compression),
  _endpoint(boost::asio::ip::address_v4::any(), _tcpPort),
  _acceptor(_ioContext) {}

TcpServer::~TcpServer() {
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

void TcpServer::startInstance(boost::system::error_code& ec) {
  _acceptor.open(_endpoint.protocol(), ec);
  if (ec)
    return;
  _acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(false), ec);
  if (ec)
    return;
  _acceptor.set_option(boost::asio::socket_base::linger(false, 0), ec);
  if (ec)
    return;
  _acceptor.bind(_endpoint, ec);
  if (ec)
    return;
  _acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
  if (ec)
    return;
  _threadPool.start(_numberThreads);
  accept();
  _thread = std::thread(&TcpServer::run, shared_from_this());
}

bool TcpServer::start(unsigned expectedNumberConnections,
		      unsigned port,
		      unsigned timeout,
		      const CompressionType& compression) {
  try {
    _instance = std::make_shared<TcpServer>(expectedNumberConnections, port, timeout, compression);
    boost::system::error_code ec;
    _instance->startInstance(ec);
    if (ec) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ' ' << ec.what() << " port=" << port << std::endl;
      return false;
    }
  }
  catch (const std::exception& e) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << " exception caught:" << e.what() << " port=" << port << std::endl;
    return false;
  }
  catch (...) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << " exception caught:" << std::strerror(errno) << std::endl;
    return false;
  }
  return true;
}

void TcpServer::stopInstance() {
  _stopped.store(true);
  boost::system::error_code ignore;
  _acceptor.close(ignore);
  _threadPool.stop();
  _ioContext.stop();
  if (_thread.joinable()) {
    _thread.join();
    std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << " ... _thread joined ..." << std::endl;
  }
}

void TcpServer::stop() {
  if (_instance)
    _instance->stopInstance();
  _instance.reset();
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

void TcpServer::pushToThreadPool(TcpConnectionPtr connection){
  _threadPool.push(connection);
}

} // end of namespace tcp
