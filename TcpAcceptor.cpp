/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpAcceptor.h"
#include "Connection.h"
#include "Server.h"
#include "ServerOptions.h"
#include "TcpSession.h"
#include "Tcp.h"

namespace tcp {

TcpAcceptor::TcpAcceptor(ServerPtr server) :
  _server(server),
  _acceptor(_ioContext) {}

TcpAcceptor::~TcpAcceptor() {
  Trace << '\n';
}

bool TcpAcceptor::start() {
  boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), ServerOptions::_tcpPort);
  boost::system::error_code ec;
  _acceptor.open(boost::asio::ip::tcp::v4(), ec);
  if (!ec)
    _acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
  if (!ec)
    _acceptor.bind(endpoint, ec);
  if (!ec)
    _acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
  if (!ec) {
    boost::asio::post(_ioContext, [this] { accept(); });
  }
  if (ec) {
    LogError << ec.what() << " tcpPort=" << ServerOptions::_tcpPort << '\n';
    return false;
  }
  return true;
}

void TcpAcceptor::stop() {
  _stopped = true;
  boost::asio::post(_ioContext, [this] () {
    if (auto self = shared_from_this(); self) {
      _ioContext.stop();
    }
  });
}

void TcpAcceptor::run() {
  try {
    _ioContext.run();
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }
}

std::tuple<HEADERTYPE, CryptoPP::SecByteBlock>
TcpAcceptor::connectionType(boost::asio::ip::tcp::socket& socket) {
  CryptoPP::SecByteBlock pubB;
  Tcp::readMsg(socket, _header, pubB);
  assert(!isCompressed(_header) && "Expected uncompressed");
  return { extractHeaderType(_header), pubB };
}

void TcpAcceptor::replyHeartbeat(boost::asio::ip::tcp::socket& socket) {
  Tcp::sendMsg(socket, _header);
  Logger logger(LOG_LEVEL::INFO, std::clog, false);
  logger << '*';
}

void TcpAcceptor::accept() {
  auto connection = std::make_shared<Connection>();
  _acceptor.async_accept(connection->_socket,
    [connection, this](boost::system::error_code ec) {
      if (_stopped)
	return;
      auto self = weak_from_this().lock();
      if (!self)
	return;
      if (!ec) {
	auto [type, pubB] = connectionType(connection->_socket);
	switch (type) {
	case HEADERTYPE::CREATE_SESSION:
	  {
	    auto session = std::make_shared<TcpSession>(_server, connection, pubB);
	    if (auto server = _server.lock(); server)
	      server->startSession(session, session);
	  }
	  break;
	case HEADERTYPE::HEARTBEAT:
	  replyHeartbeat(connection->_socket);
	  break;
	default:
	  break;
	}
	accept();
      }
      else {
	(ec == boost::asio::error::operation_aborted ? Debug : LogError) <<
	  ec.what() << '\n';
      }
    });
}

} // end of namespace tcp
