/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpAcceptor.h"
#include "ConnectionDetails.h"
#include "Logger.h"
#include "ServerOptions.h"
#include "TcpSession.h"
#include "Tcp.h"

namespace tcp {

TcpAcceptor::TcpAcceptor(const ServerOptions& options) :
  _options(options),
  _ioContext(1),
  _endpoint(boost::asio::ip::address_v4::any(), _options._tcpPort),
  _acceptor(_ioContext),
  _threadPoolSession(_options._maxTcpSessions) {}

TcpAcceptor::~TcpAcceptor() {
  Trace << std::endl;
}

bool TcpAcceptor::start() {
  boost::system::error_code ec;
  _acceptor.open(_endpoint.protocol(), ec);
  if (!ec)
    _acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
  if (!ec)
    _acceptor.bind(_endpoint, ec);
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
  boost::asio::post(_ioContext, [this] () {
    auto self = shared_from_this();
    _stopped = true;
    _ioContext.stop();
    for (auto pr : _sessions) {
      auto session = pr.second.lock();
      if (session)
	session->stop();
    }
  });
  _threadPoolAcceptor.stop();
  _threadPoolSession.stop();
}

void TcpAcceptor::run() {
  try {
    _ioContext.run();
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
  }
}

TcpAcceptor::Request TcpAcceptor::findSession(boost::asio::ip::tcp::socket& socket) {
  std::string clientId;
  auto [success, ec] = readMsg(socket, _header, clientId);
  assert(!isCompressed(_header) && "Expected uncompressed");
  if (ec) {
    LogError << ec.what() << std::endl;
    return { HEADERTYPE::ERROR, _sessions.end(), false };
  }
  HEADERTYPE type = extractHeaderType(_header);
  SessionMap::iterator it = _sessions.end();
  if (!clientId.empty()) {
    it = _sessions.find(clientId);
    if (it == _sessions.end()) {
      Info << ":related session not found" << std::endl;
      return { HEADERTYPE::ERROR, _sessions.end(), false };
    }
  }
  return { type, it, true };
}

bool TcpAcceptor::createSession(ConnectionDetailsPtr details) {
  std::ostringstream os;
  os << details->_socket.remote_endpoint() << std::flush;
  std::string clientId = os.str();
  auto session = std::make_shared<TcpSession>(_options, details, clientId);
  auto [it, inserted] = _sessions.emplace(clientId, session);
  if (!inserted) {
    LogError << "duplicate clientId" << std::endl;
    return false;
  }
  session->start();
  _threadPoolSession.push(session);
  return true;
}

void TcpAcceptor::replyHeartbeat(boost::asio::ip::tcp::socket& socket) {
  char heartbeatBuffer[HEADER_SIZE] = {};
  encodeHeader(heartbeatBuffer, _header);
  boost::system::error_code ec;
  size_t transferred[[maybe_unused]] =
    boost::asio::write(socket, boost::asio::buffer(heartbeatBuffer), ec);
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
	auto [type, it, success] = findSession(details->_socket);
	switch (type) {
	case HEADERTYPE::CREATE_SESSION:
	  filterSessions();
	  if (!createSession(details))
	    return;
	  break;
	case HEADERTYPE::HEARTBEAT:
	  replyHeartbeat(details->_socket);
	  break;
	default:
	  break;
	}
	accept();
      }
    });
}

void TcpAcceptor::filterSessions() {
  for (auto it = _sessions.begin(); it != _sessions.end();) {
    auto session = it->second.lock();
    if (!session)
      it = _sessions.erase(it);
    else
      ++it;
  }
}

} // end of namespace tcp
