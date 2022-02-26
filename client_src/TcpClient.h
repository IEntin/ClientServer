/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Client.h"
#include <boost/asio.hpp>

struct TcpClientOptions;

namespace tcp {

struct CloseSocket {
  CloseSocket(boost::asio::ip::tcp::socket& socket);
  ~CloseSocket();
  boost::asio::ip::tcp::socket& _socket;
};

class TcpClient : protected Client {

  using Batch = std::vector<std::string>;

  bool processTask(const Batch& payload);

  bool readReply(size_t uncomprSize, size_t comprSize, bool bcompressed);

  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::socket _socket;
  const TcpClientOptions& _options;
 public:
  TcpClient(const TcpClientOptions& options);
  ~TcpClient() = default;

  bool run(const Batch& payload);

};

} // end of namespace tcp
