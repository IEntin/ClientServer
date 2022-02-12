/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/asio.hpp>

struct TcpClientOptions;

namespace tcp {

using Batch = std::vector<std::string>;

struct CloseSocket {
  CloseSocket(boost::asio::ip::tcp::socket& socket);
  ~CloseSocket();
  boost::asio::ip::tcp::socket& _socket;
};

bool run(const Batch& payload,
	 const TcpClientOptions& options,
	 std::ostream* dataStream = nullptr);

bool processTask(boost::asio::ip::tcp::socket& socket,
		 const Batch& payload,
		 const TcpClientOptions& options,
		 std::ostream* dataStream = nullptr);

bool readReply(boost::asio::ip::tcp::socket& socket,
	       size_t uncomprSize,
	       size_t comprSize,
	       bool bcompressed,
	       std::ostream* pstream);

} // end of namespace tcp
