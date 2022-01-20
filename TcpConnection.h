/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Utility.h"
#include <memory>
#include <boost/asio.hpp>

namespace tcp {

using AsioTimer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
  TcpConnection(boost::asio::io_context& io_context);
  ~TcpConnection();

  void start();
  auto& socket() { return _socket; }
  auto& remoteEndpoint() { return _remoteEndpoint; }
  bool isStopped() const { return _stopped; }
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

  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::endpoint _remoteEndpoint;
  boost::asio::ip::tcp::socket _socket;
  AsioTimer _timer;
  char _header[HEADER_SIZE] = {};
  std::vector<char> _request;
  Batch _requestBatch;
  std::vector<char> _uncompressed;
  Batch _response;
  std::thread _thread;
  std::atomic<bool> _stopped = false;
  static const bool _useStringView;
};

} // end of namespace tcp
