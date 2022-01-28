/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "TcpConnection.h"

namespace tcp {

class TcpServer {
public:
  TcpServer(unsigned port, unsigned timeout);
  ~TcpServer() = default;
  void stop();
  static bool startServer(unsigned port, unsigned timeout);
  static void stopServer();
private:
  void accept();

  void handleAccept(std::shared_ptr<TcpConnection> connection,
		    const boost::system::error_code& ec);
  static void run() noexcept;

  boost::asio::io_context _ioContext;
  unsigned _tcpPort;
  unsigned _timeout;
  boost::asio::ip::tcp::endpoint _endpoint;
  boost::asio::ip::tcp::acceptor _acceptor;
  std::thread _thread;
  static std::shared_ptr<TcpServer> _instance;
};

} // end of namespace tcp
