/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Client.h"
#include <boost/asio.hpp>

namespace tcp {

class TcpClient : protected Client {

  bool send(const std::vector<char>& msg) override;

  bool receive() override;

  bool readReply(size_t uncomprSize, size_t comprSize, bool bcompressed);

  boost::asio::io_context _ioContext;

  boost::asio::ip::tcp::socket _socket;

  boost::asio::ip::tcp::endpoint _endpoint;

  std::string _clientId;

 public:

  TcpClient(const ClientOptions& options);

  ~TcpClient() override;

  bool run() override;

};

} // end of namespace tcp
