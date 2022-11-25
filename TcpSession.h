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

using TcpSessionPtr = std::shared_ptr<class TcpSession>;

using TcpHeartbeatWeakPtr = std::weak_ptr<class TcpHeartbeat>;

using TcpAcceptorPtr = std::shared_ptr<class TcpAcceptor>;

class TcpSession final : public std::enable_shared_from_this<TcpSession>, public RunnableT<TcpSession> {
  friend class TcpHeartbeat;
public:
  TcpSession(const ServerOptions& options,
	     SessionDetailsPtr details,
	     std::string_view clientId,
	     TcpAcceptorPtr parent);
  ~TcpSession() override;

  void run() noexcept override;
  bool start() override;
  void stop() override;
  void checkCapacity() override;
private:
  void readHeader();
  void readRequest();
  void write(std::string_view msg, std::function<void(TcpSession*)> nextFunc = nullptr);
  void asyncWait();
  bool onReceiveRequest();
  bool sendReply(const Response& response);
  const ServerOptions& _options;
  const std::string _clientId;
  SessionDetailsPtr _details;
  boost::asio::io_context& _ioContext;
  boost::asio::ip::tcp::socket& _socket;
  boost::asio::strand<boost::asio::io_context::executor_type> _strand;
  AsioTimer _timeoutTimer;
  char _headerBuffer[HEADER_SIZE] = {};
  HEADER _header;
  std::vector<char> _request;
  std::vector<char> _uncompressed;
  TcpAcceptorPtr _parent;
};

} // end of namespace tcp
