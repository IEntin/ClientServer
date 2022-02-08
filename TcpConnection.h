/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include "ThreadPool.h"
#include <boost/asio.hpp>
#include <memory>

using Batch = std::vector<std::string>;

namespace tcp {

using AsioTimer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;

class TcpServer;

class TcpConnection : public std::enable_shared_from_this<TcpConnection>, public Runnable {
public:
  TcpConnection(unsigned timeout, TcpServer* server);
  ~TcpConnection() override;

  void run() noexcept override;
  bool stopped() const override;
  void start();
  auto& socket() { return _socket; }
  auto& endpoint() { return _endpoint; }
private:
  void readHeader();
  void readRequest();
  void write(std::string_view reply);
  void handleReadHeader(const boost::system::error_code& ec, size_t transferred);
  void handleReadRequest(const boost::system::error_code& ec, size_t transferred);
  void handleWriteReply(const boost::system::error_code& ec, size_t transferred);
  void asyncWait();
  bool onReceiveRequest();
  bool sendReply(Batch& batch);
  bool decompress(const std::vector<char>& input, std::vector<char>& uncompressed);

  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::endpoint _endpoint;
  boost::asio::ip::tcp::socket _socket;
  unsigned _timeout;
  AsioTimer _timer;
  char _headerBuffer[HEADER_SIZE] = {};
  HEADER _header;
  std::vector<char> _request;
  std::vector<char> _uncompressed;
  Batch _response;
  TcpServer* _server;
};

} // end of namespace tcp
