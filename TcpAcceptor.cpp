/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpAcceptor.h"
#include "ServerOptions.h"
#include "SessionDetails.h"
#include "TcpHeartbeat.h"
#include "TcpSession.h"
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

unsigned TcpAcceptor::getNumberObjects() const {
  return _objectCounter._numberObjects;
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
    accept();
    _threadPoolAcceptor.push(shared_from_this());
  }
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' 
	 << ec.what() << " tcpPort=" << _options._tcpPort << std::endl;
    stop();
  }
  return !ec;
}

void TcpAcceptor::stop() {
  _ioContext.post([this] () {
    auto self = shared_from_this();
    _stopped.store(true);
    _ioContext.stop();
    for (auto pr : _sessions) {
      auto session = pr.second.lock();
      if (session)
	session->stop();
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

void TcpAcceptor::pushHeartbeat(RunnablePtr heartbeat) {
  _threadPoolHeartbeat.push(heartbeat);
}

void TcpAcceptor::accept() {
  auto details = std::make_shared<SessionDetails>();
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
	// remove dead sessions
	for (auto it = _sessions.begin(); it != _sessions.end();)
	  if (!it->second.lock())
	    it = _sessions.erase(it);
	  else
	    ++it;
	boost::system::error_code ec;
	char buffer[HEADER_SIZE] = {};
	size_t result[[maybe_unused]] =
	  boost::asio::read(details->_socket, boost::asio::buffer(buffer, HEADER_SIZE), ec);
	if (ec) {
	  CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
	  return;
	}
	HEADER header = decodeHeader(std::string_view(buffer, HEADER_SIZE));
	HEADERTYPE type = getHeaderType(header);
	std::string clientId;
	ssize_t size = getUncompressedSize(header);
	if (size > 0) {
	  std::vector<char> payload(size);
	  size_t transferred[[maybe_unused]] =
	    boost::asio::read(details->_socket, boost::asio::buffer(payload, size), ec);
	  if (ec) {
	    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
	    return;
	  }
	  clientId.assign(payload.data(), payload.size());
	}
	switch (type) {
	case HEADERTYPE::SESSION:
	  {
	    std::ostringstream os;
	    os << details->_socket.remote_endpoint() << std::flush;
	    clientId.assign(os.str());
	    auto session = std::make_shared<TcpSession>(_options, details, shared_from_this());
	    auto [it, inserted] = _sessions.emplace(clientId, session->weak_from_this());
	    if (!inserted) {
	      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
		   << "-duplicate clientId" << std::endl;
	      return;
	    }
	    session->start();
	    _threadPoolSession.push(session);
	  }
	  break;
	case HEADERTYPE::HEARTBEAT:
	  {
	    auto it = _sessions.find(clientId);
	    if (it == _sessions.end()) {
	      CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
		   << ":related session not found" << std::endl;
	      break;
	    }
	    auto session = it->second.lock();
	    if (!session) {
	      CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
		   << ":corresponding session destroyed" << std::endl;
	      break;
	    }
	    auto heartbeat = std::make_shared<TcpHeartbeat>(_options, details, it->second, shared_from_this());
	    session->setHeartbeat(heartbeat->weak_from_this());
	    heartbeat->start();
	    _threadPoolHeartbeat.push(heartbeat);
	  }
	  break;
	default:
	  break;
	}
	accept();
      }
    });
}

void TcpAcceptor::remove(RunnablePtr toRemove) {
  _threadPoolSession.removeFromQueue(toRemove);
}

} // end of namespace tcp
