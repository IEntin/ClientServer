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

using SessionDetailsPtr = std::shared_ptr<struct SessionDetails>;

class TcpSession : public std::enable_shared_from_this<TcpSession>, public Runnable {
public:
  TcpSession(const ServerOptions& options, SessionDetailsPtr details, RunnablePtr parent);
  ~TcpSession() override;

  void run() noexcept override;
  bool start() override;
  void stop() override;
private:
  void readHeader();
  void readRequest();
  void write(std::string_view msg, std::function<void(TcpSession*)> nextFunc = &TcpSession::readHeader);
  void asyncWait();
  void heartbeatWait();
  bool onReceiveRequest();
  bool sendReply(const Response& response);
  bool decompress(const std::vector<char>& input, std::vector<char>& uncompressed);
  void heartbeat();
  void heartbeatThreadFunc();
  const ServerOptions& _options;
  SessionDetailsPtr _details;
  boost::asio::io_context& _ioContext;
  boost::asio::ip::tcp::socket& _socket;
  boost::asio::strand<boost::asio::io_context::executor_type> _strand;
  AsioTimer _timer;
  AsioTimer _heartbeatTimer;
  char _headerBuffer[HEADER_SIZE] = {};
  HEADER _header;
  char _heartbeatBuffer[HEADER_SIZE] = {};
  std::vector<char> _request;
  std::vector<char> _uncompressed;
  Response _response;
  RunnablePtr _parent;
  std::thread _heartbeatThread;
  std::atomic<PROBLEMS> _problem = PROBLEMS::NONE;
  std::chrono::time_point<std::chrono::steady_clock> _currentHeartbeat;
  std::chrono::time_point<std::chrono::steady_clock> _nextHeartbeat;
};

} // end of namespace tcp
