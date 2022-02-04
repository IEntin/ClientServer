/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/asio.hpp>
#include <memory>

namespace tcp {

using TcpServerPtr = std::shared_ptr<class TcpServer>;

class TcpServer : public std::enable_shared_from_this<TcpServer> {
public:
  TcpServer(unsigned port, unsigned timeout);
  ~TcpServer() = default;
  void start();
  void stop();
  bool stopped() const { return _stopped; }
  std::vector<std::thread>& getConnectionThreadVector() { return _connectionThreads; }
private:
  void accept();

  void handleAccept(std::shared_ptr<class TcpConnection> connection,
		    const boost::system::error_code& ec);
  void run() noexcept;

  boost::asio::io_context _ioContext;
  unsigned _tcpPort;
  unsigned _timeout;
  boost::asio::ip::tcp::endpoint _endpoint;
  boost::asio::ip::tcp::acceptor _acceptor;
  std::atomic<bool> _stopped = false;
  std::thread _thread;
  std::vector<std::thread> _connectionThreads;
};

} // end of namespace tcp
