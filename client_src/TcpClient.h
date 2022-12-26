/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Client.h"
#include <boost/asio.hpp>

namespace tcp {

class TcpClient : public Client {

  bool send(const std::vector<char>& msg) override;

  bool receive() override;

  bool receiveStatus() override;

  bool destroySession() override;

  bool readReply(const HEADER& header);

  static void waitHandler(const boost::system::error_code& ec);

  boost::asio::io_context _ioContext;

  boost::asio::ip::tcp::socket _socket;

 public:

  TcpClient(const ClientOptions& options);

  ~TcpClient() override;

  bool run() override;

};

} // end of namespace tcp
