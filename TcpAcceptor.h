/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <cryptopp/secblock.h>
#include <boost/asio.hpp>

#include "Runnable.h"

class Server;
using ServerPtr = std::shared_ptr<Server>;
using ServerWeakPtr = std::weak_ptr<Server>;

namespace tcp {

class TcpAcceptor : public std::enable_shared_from_this<TcpAcceptor>,
  public Runnable {
 public:
  explicit TcpAcceptor(ServerPtr server);
  ~TcpAcceptor() override;
private:
  void run() override;
  bool start() override;
  void stop() override;

  void accept();
  std::tuple<HEADERTYPE, CryptoPP::SecByteBlock> connectionType(boost::asio::ip::tcp::socket& socket);
  void replyHeartbeat(boost::asio::ip::tcp::socket& socket);

  ServerWeakPtr _server;
  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::acceptor _acceptor;
  HEADER _header;
};

} // end of namespace tcp
