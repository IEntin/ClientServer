/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/asio.hpp>

struct TcpClientOptions;

namespace tcp {

struct CloseSocket {
  CloseSocket(boost::asio::ip::tcp::socket& socket);
  ~CloseSocket();
  boost::asio::ip::tcp::socket& _socket;
};

class TcpClient {

  TcpClient() = delete;
  ~TcpClient() = delete;

  using Batch = std::vector<std::string>;

  static bool processTask(boost::asio::ip::tcp::socket& socket,
			  const Batch& payload,
			  const TcpClientOptions& options);

  static bool readReply(boost::asio::ip::tcp::socket& socket,
			size_t uncomprSize,
			size_t comprSize,
			bool bcompressed,
			std::ostream* pstream);
 public:
  static bool run(const Batch& payload, const TcpClientOptions& options);

};

} // end of namespace tcp
