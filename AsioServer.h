/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Utility.h"
#include <list>
#include <memory>
#include <boost/asio.hpp>

using AsioTimer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;

class Session : public std::enable_shared_from_this<Session> {
public:
  Session(boost::asio::ip::tcp::socket socket);

  void start();
private:
  void readHeader();

  void readRequest();

  void writeReply();

  void handleReadHeader(const boost::system::error_code& ec, size_t transferred);

  void handleReadRequest(const boost::system::error_code& ec, size_t transferred);

  void handleWriteReply(const boost::system::error_code& ec, size_t transferred);

  void asyncWait();

  boost::asio::ip::tcp::socket _socket;
  AsioTimer _timer;
  char _header[HEADER_SIZE] = {};
  std::vector<char> _request;
};

class AsioServer {
public:
  AsioServer(boost::asio::io_context& io_context,
	     const boost::asio::ip::tcp::endpoint& endpoint);
  static bool startServers();
  static void joinThread();
private:
  void accept();

  static void run();

  boost::asio::ip::tcp::acceptor _acceptor;

  static boost::asio::io_context _ioContext;

  static std::thread _thread;

  static std::list<AsioServer> _servers;
};
