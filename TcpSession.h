/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/container/static_vector.hpp>

#include "Runnable.h"
#include <boost/asio.hpp>

using Response = std::vector<std::string>;
using TaskPtr = std::shared_ptr<class Task>;

namespace tcp {

using AsioTimer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;

class TcpSession final : public std::enable_shared_from_this<TcpSession>, public RunnableT<TcpSession> {
public:
  TcpSession();
  ~TcpSession() override;

  boost::asio::ip::tcp::socket& socket() { return _socket; }
private:
  bool start() override;
  void run() noexcept override;
  void stop() override;
  bool sendStatusToClient() override;
  std::string_view getId() override { return _clientId; }
  std::string_view getDisplayName() const override{ return "tcp"; }
  void readHeader();
  void readRequest(const HEADER& header);
  void write(const HEADER& header, std::string_view msg);
  void asyncWait();
  bool sendReply();
  std::string _request;
  std::string _clientId;
  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::socket _socket;
  AsioTimer _timeoutTimer;
  char _headerBuffer[HEADER_SIZE] = {};
  Response _response;
  TaskPtr _task;
  boost::container::static_vector<boost::asio::const_buffer, 2> _asioBuffers;
};

} // end of namespace tcp
