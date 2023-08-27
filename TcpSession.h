/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <boost/asio.hpp>

using Response = std::vector<std::string>;
struct ServerOptions;

namespace tcp {

using AsioTimer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;

class TcpSession final : public std::enable_shared_from_this<TcpSession>, public RunnableT<TcpSession> {
public:
  TcpSession(const ServerOptions& options);
  ~TcpSession() override;

  boost::asio::ip::tcp::socket& socket() { return _socket; }
private:
  bool start() override;
  void run() noexcept override;
  void stop() override;
  bool sendStatusToClient() override;
  std::string_view getId() override { return _clientId; }
  void readHeader();
  void readRequest();
  void write(std::string_view msg);
  void asyncWait();
  bool sendReply();
  const ServerOptions& _options;
  std::string _clientId;
  static inline std::string_view _displayType = "tcp";
  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::socket _socket;
  AsioTimer _timeoutTimer;
  char _headerBuffer[HEADER_SIZE] = {};
  HEADER _header;
  std::string _request;
  Response _response;
  std::vector<boost::asio::const_buffer> _asioBuffers;
};

} // end of namespace tcp
