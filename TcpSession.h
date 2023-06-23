/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <boost/asio.hpp>

using Response = std::vector<std::string>;
class Server;
struct ServerOptions;
struct CryptoKeys;

namespace tcp {

using AsioTimer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;
using ConnectionDetailsPtr = std::shared_ptr<struct ConnectionDetails>;

class TcpSession final : public std::enable_shared_from_this<TcpSession>, public RunnableT<TcpSession> {
public:
  TcpSession(const Server& server, ConnectionDetailsPtr details, std::string_view clientId);
  ~TcpSession() override;

private:
  bool start() override { return true; }
  void run() noexcept override;
  void stop() override;
  bool sendStatusToClient() override;
  void readHeader();
  void readRequest();
  void write(std::string_view msg);
  void asyncWait();
  bool sendReply(const Response& response);
  const ServerOptions& _options;
  const std::string _clientId;
  const CryptoKeys& _cryptoKeys;
  static inline std::string_view _name = "tcp";
  ConnectionDetailsPtr _details;
  boost::asio::io_context& _ioContext;
  boost::asio::ip::tcp::socket _socket;
  AsioTimer _timeoutTimer;
  char _headerBuffer[HEADER_SIZE] = {};
  HEADER _header;
  std::vector<char> _request;
};

} // end of namespace tcp
