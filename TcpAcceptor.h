/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Acceptor.h"
#include <boost/asio.hpp>

namespace tcp {

using ConnectionDetailsPtr = std::shared_ptr<struct ConnectionDetails>;

class TcpAcceptor : public std::enable_shared_from_this<TcpAcceptor>,
  public virtual Acceptor {
 public:
  TcpAcceptor(const ServerOptions& options,
	      ThreadPoolBase& threadPoolAcceptor,
	      ThreadPoolDiffObj& threadPoolSession);
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
  void createSession(ConnectionDetailsPtr details);
  void replyHeartbeat(boost::asio::ip::tcp::socket& socket);

  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::acceptor _acceptor;
  HEADER _header;
};

} // end of namespace tcp
