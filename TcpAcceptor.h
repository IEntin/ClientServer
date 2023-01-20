/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPool.h"
#include <boost/asio.hpp>
#include <map>

struct ServerOptions;

class Server;

using SessionMap = std::map<std::string, RunnableWeakPtr>;

namespace tcp {

using ConnectionDetailsPtr = std::shared_ptr<struct ConnectionDetails>;

class TcpAcceptor : public std::enable_shared_from_this<TcpAcceptor>,
  public RunnableT<TcpAcceptor> {
 public:
  TcpAcceptor(const ServerOptions& options, Server& server);

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

  bool createSession(ConnectionDetailsPtr details);

  void destroySession(const std::string& clientId);

  void replyHeartbeat(boost::asio::ip::tcp::socket& socket);

  const ServerOptions& _options;
  Server& _server;
  SessionMap _sessions;
  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::acceptor _acceptor;
  HEADER _header;
  ThreadPool _threadPoolAcceptor;
  ThreadPool _threadPoolSession;
};

} // end of namespace tcp
