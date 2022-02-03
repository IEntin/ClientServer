/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include <memory>
#include <boost/asio.hpp>

using Batch = std::vector<std::string>;

namespace tcp {

using AsioTimer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
  TcpConnection(boost::asio::io_context& io_context, unsigned timeout);
  ~TcpConnection();

  void start();
  auto& socket() { return _socket; }
  auto& endpoint() { return _endpoint; }
private:
  void run() noexcept;
  void readHeader();
  void readRequest();
  void write(std::string_view reply);
  void handleReadHeader(const boost::system::error_code& ec, size_t transferred);
  void handleReadRequest(const boost::system::error_code& ec, size_t transferred);
  void handleWriteReply(const boost::system::error_code& ec, size_t transferred);
  void asyncWait();
  bool onReceiveRequest();
  bool sendReply(Batch& batch);

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
  std::thread _thread;
};

} // end of namespace tcp
