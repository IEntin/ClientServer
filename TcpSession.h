/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include "Runnable.h"
#include <boost/asio.hpp>

using Response = std::vector<std::string>;

struct ServerOptions;

class SessionContainer;

namespace tcp {

using AsioTimer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;

using ConnectionDetailsPtr = std::shared_ptr<struct ConnectionDetails>;

class TcpSession final : public std::enable_shared_from_this<TcpSession>, public RunnableT<TcpSession> {
public:
  TcpSession(const ServerOptions& options,
	     ConnectionDetailsPtr details,
	     std::string_view clientId,
	     SessionContainer& sessionContainer);
  ~TcpSession() override;

private:
  void run() noexcept override;
  bool start() override;
  void stop() override;
  void checkCapacity() override;
  void notify() override;
  void readHeader();
  void readRequest();
  void write(std::string_view msg, std::function<void(TcpSession*)> nextFunc = nullptr);
  void asyncWait();
  bool onReceiveRequest();
  bool sendReply(const Response& response);
  const ServerOptions& _options;
  SessionContainer& _sessionContainer;
  const std::string _clientId;
  ConnectionDetailsPtr _details;
  boost::asio::io_context& _ioContext;
  boost::asio::ip::tcp::socket& _socket;
  boost::asio::strand<boost::asio::io_context::executor_type> _strand;
  AsioTimer _timeoutTimer;
  char _headerBuffer[HEADER_SIZE] = {};
  HEADER _header;
  std::vector<char> _request;
  std::vector<char> _uncompressed;
};

} // end of namespace tcp
