/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include "Runnable.h"
#include <boost/asio.hpp>
#include <thread>

using Response = std::vector<std::string>;

struct ServerOptions;

namespace tcp {

using AsioTimer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;

using SessionDetailsPtr = std::shared_ptr<struct SessionDetails>;

using TcpServerPtr = std::shared_ptr<class TcpServer>;

using TcpSessionPtr = std::shared_ptr<class TcpSession>;

class TcpSession : public std::enable_shared_from_this<TcpSession>, public Runnable {
public:
  TcpSession(const ServerOptions& options,
	     SessionDetailsPtr details,
	     std::string_view clientId,
	     TcpServerPtr parent);
  ~TcpSession() override;

  void run() noexcept override;
  bool start() override;
  void stop() override;

  const std::string_view getClientId() const { return _clientId; }
private:
  void readHeader();
  void readRequest();
  void write(std::string_view msg, std::function<void(TcpSession*)> nextFunc = nullptr);
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
  AsioTimer _timeoutTimer;
  AsioTimer _heartbeatTimer;
  unsigned _heartbeatPeriod;
  char _headerBuffer[HEADER_SIZE] = {};
  HEADER _header;
  char _heartbeatBuffer[HEADER_SIZE] = {};
  std::vector<char> _request;
  std::vector<char> _uncompressed;
  Response _response;
  const std::string _clientId;
  TcpServerPtr _parent;
  std::jthread _heartbeatThread;
  std::atomic<PROBLEMS> _problem = PROBLEMS::NONE;
};

} // end of namespace tcp
