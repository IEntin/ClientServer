/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <boost/asio.hpp>

class Server;
struct ServerOptions;

namespace tcp {

using ContextPtr = std::shared_ptr<boost::asio::io_context>;
using SocketPtr = std::shared_ptr<boost::asio::ip::tcp::socket>;

class TcpAcceptor : public std::enable_shared_from_this<TcpAcceptor>,
  public RunnableT<TcpAcceptor> {
 public:
  TcpAcceptor(Server& server);
  ~TcpAcceptor() override;
private:
  struct Request {
    HEADERTYPE _type;
    std::string _clientId;
    bool _success;
  };

  void run() override;
  bool start() override;
  void stop() override;

  void accept();
  Request receiveRequest(boost::asio::ip::tcp::socket& socket);
  void createSession(ContextPtr contextPtr, SocketPtr socketPtr);
  void replyHeartbeat(boost::asio::ip::tcp::socket& socket);

  Server& _server;
  const ServerOptions& _options;
  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::acceptor _acceptor;
  HEADER _header;
};

} // end of namespace tcp
