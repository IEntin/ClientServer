/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/asio.hpp>

#include "Runnable.h"
#include "Session.h"

namespace tcp {

using AsioTimer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;

using ConnectionPtr = std::shared_ptr<struct Connection>;

class TcpSession final : public std::enable_shared_from_this<TcpSession>,
			 public RunnableT<TcpSession>,
			 public Session {
public:
  TcpSession(ServerWeakPtr server,
	     ConnectionPtr connection,
	     std::string_view Bstring);
  ~TcpSession() override;

  boost::asio::ip::tcp::socket& socket() { return _socket; }
private:
  bool start() override;
  void run() noexcept override;
  void stop() override;
  bool sendStatusToClient() override;
  std::size_t getId() override { return _clientId; }
  std::string_view getDisplayName() const override{ return "tcp"; }
  void readHeader();
  void readRequest(const HEADER& header);
  void write(const HEADER& header, std::string_view msg);
  void asyncWait();
  bool sendReply();
  ConnectionPtr _connection;
  boost::asio::io_context& _ioContext;
  boost::asio::ip::tcp::socket _socket;
  AsioTimer _timeoutTimer;
  char _headerBuffer[HEADER_SIZE] = {};
};

} // end of namespace tcp
