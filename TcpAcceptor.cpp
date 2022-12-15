/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpAcceptor.h"
#include "ConnectionDetails.h"
#include "ServerOptions.h"
#include "TcpHeartbeat.h"
#include "TcpSession.h"
#include "Tcp.h"
#include "Utility.h"

namespace tcp {

TcpAcceptor::TcpAcceptor(const ServerOptions& options) :
  _options(options),
  _ioContext(1),
  _endpoint(boost::asio::ip::address_v4::any(), _options._tcpPort),
  _acceptor(_ioContext),
  _threadPoolSession(_options._maxTcpSessions) {}

TcpAcceptor::~TcpAcceptor() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
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
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' 
	 << ec.what() << " tcpPort=" << _options._tcpPort << std::endl;
    return false;
  }
  return true;
}

void TcpAcceptor::stop() {
  boost::asio::post(_ioContext, [this] () {
    auto self = shared_from_this();
    _stopped.store(true);
    _ioContext.stop();
    for (auto pr : _sessions) {
      auto session = pr.second.lock();
      if (session)
	session->stop();
    }
    for (auto pr : _heartbeats) {
      auto heartbeat = pr.second.lock();
      if (heartbeat)
	heartbeat->stop();
    }
  });
  _threadPoolAcceptor.stop();
  _threadPoolHeartbeat.stop();
  _threadPoolSession.stop();
}

void TcpAcceptor::run() {
  try {
    _ioContext.run();
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << std::endl;
  }
}

TcpAcceptor::Request TcpAcceptor::findSession(boost::asio::ip::tcp::socket& socket) {
  HEADER header;
  std::vector<char> payload;
  auto [success, ec] = readMsg(socket, header, payload);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
    return { HEADERTYPE::ERROR, _sessions.end(), false };
  }
  HEADERTYPE type = getHeaderType(header);
  std::string clientId;
  ConnectionMap::iterator it;
  if (!payload.empty()) {
    clientId.assign(payload.data(), payload.size());
    
    ConnectionMap& connections =
      (type == HEADERTYPE::CREATE_HEARTBEAT || type == HEADERTYPE::DESTROY_HEARTBEAT) ?
      _heartbeats : _sessions;
    it = connections.find(clientId);
    if (it == connections.end()) {
      CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
	   << ":related connection not found" << std::endl;
      return { HEADERTYPE::ERROR, connections.end(), false };
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
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << "-duplicate clientId" << std::endl;
    return false;
  }
  session->start();
  _threadPoolSession.push(session);
  return true;
}

bool TcpAcceptor::createHeartbeat(ConnectionDetailsPtr details) {
  std::ostringstream os;
  os << details->_socket.remote_endpoint() << std::flush;
  std::string clientId = os.str();
  auto heartbeat = std::make_shared<TcpHeartbeat>(details, clientId);
  auto [it, inserted] = _heartbeats.emplace(clientId, heartbeat);
  if (!inserted) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << "-duplicate clientId" << std::endl;
    return false;
  }
  heartbeat->start();
  _threadPoolHeartbeat.push(heartbeat);
  return true;
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
      if (ec)
	(ec == boost::asio::error::operation_aborted ? CLOG : CERR)
	  << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
      else {
	auto [type, it, success] = findSession(details->_socket);
	switch (type) {
	case HEADERTYPE::CREATE_SESSION:
	  if (!createSession(details))
	    return;
	  break;
	case HEADERTYPE::DESTROY_SESSION:
	  {
	    auto session = it->second.lock();
	    if (session) {
	      session->stop();
	      _threadPoolSession.removeFromQueue(session);
	    }
	    _sessions.erase(it);
	  }
	  break;
	case HEADERTYPE::CREATE_HEARTBEAT:
	  if (!createHeartbeat(details))
	    return;
	  break;
	case HEADERTYPE::DESTROY_HEARTBEAT:
	  {
	    auto heartbeat = it->second.lock();
	    if (heartbeat)
	      heartbeat->stop();
	    _heartbeats.erase(it);
	  }
	  break;
	default:
	  break;
	}
	accept();
      }
    });
}

} // end of namespace tcp
