/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "TcpConnection.h"
#include <set>

namespace tcp {

class TcpServer {
public:
  TcpServer(const boost::asio::ip::tcp::endpoint& endpoint);
  static bool startServer();
  static void stopServer();
private:
  void accept();

  void handleAccept(std::shared_ptr<TcpConnection> connection,
		    const boost::system::error_code& ec);
  static void filterConnections();

  static void run() noexcept;
  static boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::acceptor _acceptor;
  static std::thread _thread;
  static std::shared_ptr<TcpServer> _server;
  static std::set<std::shared_ptr<TcpConnection>> _connections;
};

} // end of namespace tcp
