/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpAcceptor.h"
#include "ConnectionDetails.h"
#include "Logger.h"
#include "Server.h"
#include "ServerOptions.h"
#include "TcpSession.h"
#include "Tcp.h"

namespace tcp {

TcpAcceptor::TcpAcceptor(Server& server) :
  _server(server),
  _options(_server.getOptions()),
  _threadPoolAcceptor(server.getThreadPoolAcceptor()),
  _ioContext(1),
  _acceptor(_ioContext) {}

TcpAcceptor::~TcpAcceptor() {
  Trace << std::endl;
}

bool TcpAcceptor::start() {
  boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), _options._tcpPort);
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
    _threadPoolAcceptor.push(shared_from_this());
  }
  if (ec) {
    LogError << ec.what() << " tcpPort=" << _options._tcpPort << std::endl;
    return false;
  }
  return true;
}

void TcpAcceptor::stop() {
  _stopped = true;
  boost::asio::post(_ioContext, [this] () {
    auto self = shared_from_this();
    _ioContext.stop();
    _server.stopSessions();
  });
}

void TcpAcceptor::run() {
  try {
    _ioContext.run();
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
  }
}

TcpAcceptor::Request TcpAcceptor::receiveRequest(boost::asio::ip::tcp::socket& socket) {
  std::string clientId;
  auto [success, ec] = Tcp::readMsg(socket, _header, clientId);
  assert(!isCompressed(_header) && "Expected uncompressed");
  if (ec) {
    LogError << ec.what() << std::endl;
    return { HEADERTYPE::ERROR, clientId, false };
  }
  HEADERTYPE type = extractHeaderType(_header);
  return { type, clientId, true };
}

void TcpAcceptor::createSession(ConnectionDetailsPtr details) {
  std::ostringstream os;
  os << details->_socket.remote_endpoint() << std::flush;
  std::string clientId = os.str();
  RunnablePtr session =
    std::make_shared<TcpSession>(_server, details, clientId);
  _server.startSession(clientId, session);
}

void TcpAcceptor::replyHeartbeat(boost::asio::ip::tcp::socket& socket) {
  auto [success, ec] = Tcp::sendMsg(socket, _header);
  if (ec) {
    LogError << ec.what() << std::endl;
    return;
  }
  Logger(LOG_LEVEL::INFO, std::clog, false) << "*" << std::flush;
}

void TcpAcceptor::accept() {
  auto details = std::make_shared<ConnectionDetails>();
  auto weak = weak_from_this();
  _acceptor.async_accept(details->_socket,
			 details->_endpoint,
    [details, this, weak](boost::system::error_code ec) {
      if (_stopped)
	return;
      auto self = weak.lock();
      if (!self)
	return;
      if (ec) {
	bool berror = ec != boost::asio::error::operation_aborted;
	Logger logger(berror ? LOG_LEVEL::ERROR : LOG_LEVEL::DEBUG, berror ? std::cerr : std::clog);
	logger << CODELOCATION << ':' << ec.what() << std::endl;
      }
      else {
	auto [type, clientId, success] = receiveRequest(details->_socket);
	if (!success)
	  return;
	switch (type) {
	case HEADERTYPE::CREATE_SESSION:
	  createSession(details);
	  break;
	case HEADERTYPE::DESTROY_SESSION:
	  _server.destroySession(clientId);
	  break;
	case HEADERTYPE::HEARTBEAT:
	  if (!_stopped)
	    replyHeartbeat(details->_socket);
	  break;
	default:
	  break;
	}
	accept();
      }
    });
}

} // end of namespace tcp
