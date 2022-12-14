/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include "Runnable.h"
#include <boost/asio.hpp>

namespace tcp {

using ConnectionDetailsPtr = std::shared_ptr<struct ConnectionDetails>;

class TcpHeartbeat final : public std::enable_shared_from_this<TcpHeartbeat>,
  public RunnableT<TcpHeartbeat> {

 public:

  TcpHeartbeat(ConnectionDetailsPtr details, std::string_view clientId);
  ~TcpHeartbeat() override;

  bool start() override;
  void stop() override;

 private:

  void run() noexcept override;

  void write();

  void read();

  const std::string _clientId;
  ConnectionDetailsPtr _details;
  boost::asio::io_context& _ioContext;
  boost::asio::ip::tcp::socket& _socket;
  char _heartbeatBuffer[HEADER_SIZE] = {};
};

} // end of namespace tcp
