/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "TcpConnection.h"

namespace tcp {

class TcpServer {
public:
  TcpServer() = delete;
  ~TcpServer() = delete;
  static bool startServer();
  static void stopServer();
private:
  static void accept();

  static void handleAccept(std::shared_ptr<TcpConnection> connection,
			   const boost::system::error_code& ec);
  static void run() noexcept;
  static boost::asio::io_context _ioContext;
  static unsigned _tcpPort;
  static boost::asio::ip::tcp::endpoint _endpoint;
  static boost::asio::ip::tcp::acceptor _acceptor;
  static std::thread _thread;
};

} // end of namespace tcp
