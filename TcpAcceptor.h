/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPool.h"
#include <boost/asio.hpp>
#include <map>

struct ServerOptions;

namespace tcp {

using ConnectionMap = std::map<std::string, RunnableWeakPtr>;

using ConnectionDetailsPtr = std::shared_ptr<struct ConnectionDetails>;

class TcpAcceptor : public std::enable_shared_from_this<TcpAcceptor>,
  public RunnableT<TcpAcceptor> {
 public:
  explicit TcpAcceptor(const ServerOptions& options);

  ~TcpAcceptor() override;

  bool start() override;

  void stop() override;

private:

  struct Request {
    HEADERTYPE _type;
    ConnectionMap::iterator _iterator;
    bool _success;
  };
  void accept();

  void run() override;

  Request findSession(boost::asio::ip::tcp::socket& socket);

  bool createSession(ConnectionDetailsPtr details);

  bool createHeartbeat(ConnectionDetailsPtr details);

  const ServerOptions& _options;
  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::endpoint _endpoint;
  boost::asio::ip::tcp::acceptor _acceptor;
  ThreadPool _threadPoolAcceptor;
  ThreadPool _threadPoolSession;
  ThreadPool _threadPoolHeartbeat;
  ConnectionMap _sessions;
  ConnectionMap _heartbeats;
};

} // end of namespace tcp
