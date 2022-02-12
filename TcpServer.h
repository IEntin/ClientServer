/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPool.h"
#include <boost/asio.hpp>
#include <memory>

enum class COMPRESSORS : unsigned short;

namespace tcp {

using TcpConnectionPtr = std::shared_ptr<class TcpConnection>;

class TcpServer {
public:
  TcpServer(unsigned expectedNumberConnections,
	    unsigned port,
	    unsigned timeout,
	    const std::pair<COMPRESSORS, bool>& compression);
  ~TcpServer();
  const std::pair<COMPRESSORS, bool>& getCompression() const { return _compression; }
  void stop();
  bool stopped() const { return _stopped; }
  void pushConnection(TcpConnectionPtr connection);
private:
  void accept();

  void handleAccept(TcpConnectionPtr connection,
		    const boost::system::error_code& ec);
  void run() noexcept;

  boost::asio::io_context _ioContext;
  unsigned _tcpPort;
  unsigned _timeout;
  std::pair<COMPRESSORS, bool> _compression;
  boost::asio::ip::tcp::endpoint _endpoint;
  boost::asio::ip::tcp::acceptor _acceptor;
  std::atomic<bool> _stopped = false;
  std::thread _thread;
  ThreadPool _connectionThreadPool;
};

} // end of namespace tcp
