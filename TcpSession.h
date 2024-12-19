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
	     const CryptoPP::SecByteBlock& pubB,
	     std::span<uint8_t> signatureWithPubKey);
  ~TcpSession() override;

private:
  bool start() override;
  void run() noexcept override;
  void stop() override;
  void sendStatusToClient() override;
  void displayCapacityCheck(std::atomic<unsigned>& totalNumberObjects) const override;
  std::string_view getDisplayName() const override{ return "tcp"; }
  void readRequest();
  void write(std::string_view payload);
  void asyncWait();
  bool sendReply();
  ConnectionPtr _connection;
  boost::asio::io_context& _ioContext;
  boost::asio::ip::tcp::socket _socket;
  boost::asio::deadline_timer _timeoutTimer;
};

} // end of namespace tcp
