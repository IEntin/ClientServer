/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Utility.h"
#include <memory>
#include <set>
#include <boost/asio.hpp>

namespace tcp {

using AsioTimer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
  explicit TcpConnection(boost::asio::io_context& io_context);
  ~TcpConnection();

  void start();
  auto& socket() { return _socket; }
  auto& endpoint() { return _endpoint; }
  static void insert(std::shared_ptr<TcpConnection> connection);
  static void destroy();
private:
  void run() noexcept;
  void readHeader();
  void readRequest();
  void write(std::string_view reply);
  void handleReadHeader(const boost::system::error_code& ec, size_t transferred);
  void handleReadRequest(const boost::system::error_code& ec, size_t transferred);
  void handleWriteReply(const boost::system::error_code& ec, size_t transferred);
  void asyncWait();
  bool decompress(size_t uncomprSize);
  bool onReceiveRequest();
  bool sendReply(Batch& batch);
  void stop();

  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::endpoint _endpoint;
  boost::asio::ip::tcp::socket _socket;
  AsioTimer _timer;
  char _header[HEADER_SIZE] = {};
  std::vector<char> _request;
  Batch _requestBatch;
  std::vector<char> _uncompressed;
  Batch _response;
  std::thread _thread;
  static std::set<std::shared_ptr<TcpConnection>> _connections;
  static const bool _useStringView;
  static std::mutex _mutex;
};

} // end of namespace tcp
