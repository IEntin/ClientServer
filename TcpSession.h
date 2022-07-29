/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include "Runnable.h"
#include <boost/asio.hpp>

using Response = std::vector<std::string>;

struct ServerOptions;

namespace tcp {

using AsioTimer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;

class TcpSession : public std::enable_shared_from_this<TcpSession>, public Runnable {
public:
  TcpSession(const ServerOptions& options, RunnablePtr parent);
  ~TcpSession() override;

  void run() noexcept override;
  bool start() override;
  void stop() override;
  auto& socket() { return _socket; }
  auto& endpoint() { return _endpoint; }
private:
  void readHeader();
  void readRequest();
  void write(std::string_view reply);
  void asyncWait();
  bool onReceiveRequest();
  bool sendReply(const Response& response);
  bool decompress(const std::vector<char>& input, std::vector<char>& uncompressed);
  const ServerOptions& _options;
  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::endpoint _endpoint;
  boost::asio::ip::tcp::socket _socket;
  int _timeout;
  AsioTimer _timer;
  char _headerBuffer[HEADER_SIZE] = {};
  HEADER _header;
  std::vector<char> _request;
  std::vector<char> _uncompressed;
  Response _response;
  COMPRESSORS _compressor;
  RunnablePtr _parent;
};

} // end of namespace tcp
