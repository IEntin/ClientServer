/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Utility.h"
#include <list>
#include <memory>
#include <boost/asio.hpp>

namespace tcp {

using AsioTimer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;

class Session : public std::enable_shared_from_this<Session> {
public:
  Session(const std::string& port, boost::asio::ip::tcp::socket socket);

  void start();
private:
  void readHeader();

  void readRequest();

  void write(std::string_view reply);

  void handleReadHeader(const boost::system::error_code& ec, size_t transferred);

  void handleReadRequest(const boost::system::error_code& ec, size_t transferred);

  void handleWriteReply(const boost::system::error_code& ec, size_t transferred);

  void asyncWait();

  bool decompress(size_t uncomprSize, std::vector<char>& uncompressed);

  bool onReceiveRequest();

  bool sendReply(Batch& batch);

  const std::string _port;
  boost::asio::ip::tcp::socket _socket;
  AsioTimer _timer;
  char _header[HEADER_SIZE] = {};
  std::vector<char> _request;
  static const bool _useStringView;
};

class TcpServer {
public:
  TcpServer(const std::string& port,
	    boost::asio::io_context& io_context,
	    const boost::asio::ip::tcp::endpoint& endpoint);
  static bool startServers();
  static void joinThread();
private:
  void accept();

  static void run();

  const std::string _port;

  boost::asio::ip::tcp::acceptor _acceptor;

  static boost::asio::io_context _ioContext;

  static std::thread _thread;

  static std::list<TcpServer> _servers;
};

} // end of namespace tcp
