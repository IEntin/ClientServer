/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <boost/asio.hpp>

class Server;

namespace tcp {

class TcpAcceptor : public std::enable_shared_from_this<TcpAcceptor>,
  public Runnable {
 public:
  TcpAcceptor(Server& server);
  ~TcpAcceptor() override;
private:
  void run() override;
  bool start() override;
  void stop() override;
  unsigned getNumberObjects() const override { return 1; }
  unsigned getNumberRunningByType() const override { return 1; }
  void displayCapacityCheck(std::atomic<unsigned>&) const override {}

  void accept();
  HEADERTYPE connectionType(boost::asio::ip::tcp::socket& socket);
  void replyHeartbeat(boost::asio::ip::tcp::socket& socket);

  Server& _server;
  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::acceptor _acceptor;
  HEADER _header;
};

} // end of namespace tcp
