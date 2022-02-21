/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPool.h"
#include "Compression.h"
#include <boost/asio.hpp>
#include <memory>

namespace tcp {

using TcpServerPtr = std::shared_ptr<class TcpServer>;

using TcpConnectionPtr = std::shared_ptr<class TcpConnection>;

class TcpServer : public std::enable_shared_from_this<TcpServer> {
public:
  TcpServer(unsigned expectedNumberConnections,
	    unsigned port,
	    unsigned timeout,
	    const CompressionType& compression);
  ~TcpServer();
  const CompressionType& getCompression() const { return _compression; }
  static bool start(unsigned expectedNumberConnections,
		    unsigned port,
		    unsigned timeout,
		    const CompressionType& compression);
  static void stop();
  bool stopped() const { return _stopped; }
  void pushToThreadPool(TcpConnectionPtr connection);
private:
  void accept();

  void handleAccept(TcpConnectionPtr connection,
		    const boost::system::error_code& ec);
  void run() noexcept;

  void startInstance(boost::system::error_code& ec);

  void stopInstance();

  const size_t _numberThreads;
  boost::asio::io_context _ioContext;
  unsigned _tcpPort;
  unsigned _timeout;
  CompressionType _compression;
  boost::asio::ip::tcp::endpoint _endpoint;
  boost::asio::ip::tcp::acceptor _acceptor;
  std::atomic<bool> _stopped = false;
  std::thread _thread;
  ThreadPool _threadPool;
  static TcpServerPtr _instance;
};

} // end of namespace tcp
