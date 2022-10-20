/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Client.h"
#include <boost/asio.hpp>

namespace tcp {

using TcpClientHeartbeatPtr = std::shared_ptr<class TcpClientHeartbeat>;

class TcpClient : protected Client {

  bool send(const std::vector<char>& msg) override;

  bool receive() override;

  bool readReply(const HEADER& header);

  boost::asio::io_context _ioContext;

  boost::asio::ip::tcp::socket _socket;

  TcpClientHeartbeatPtr _heartbeat;

 public:

  TcpClient(const ClientOptions& options);

  ~TcpClient() override;

  bool run() override;

};

} // end of namespace tcp
