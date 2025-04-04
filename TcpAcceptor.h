/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/asio.hpp>

#include <cryptopp/secblock.h>

#include "Runnable.h"

using ServerWeakPtr = std::weak_ptr<class Server>;

namespace tcp {

class TcpAcceptor : public std::enable_shared_from_this<TcpAcceptor>,
  public Runnable {
 public:
  explicit TcpAcceptor(ServerWeakPtr server);
  ~TcpAcceptor() override = default;
private:
  void run() override;
  bool start() override;
  void stop() override;

  void accept();
  std::tuple<HEADERTYPE, std::string, CryptoPP::SecByteBlock, std::string>
  connectionType(boost::asio::ip::tcp::socket& socket);
  void replyHeartbeat(boost::asio::ip::tcp::socket& socket);

  ServerWeakPtr _server;
  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::acceptor _acceptor;
  HEADER _header;
};

} // end of namespace tcp
