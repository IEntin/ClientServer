/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpAcceptor.h"
#include "ServerOptions.h"
#include "SessionDetails.h"
#include "TaskController.h"
#include "TcpHeartbeat.h"
#include "TcpSession.h"
#include "Utility.h"

namespace tcp {

TcpAcceptor::TcpAcceptor(const ServerOptions& options) :
  _options(options),
  _ioContext(1),
  _endpoint(boost::asio::ip::address_v4::any(), _options._tcpPort),
  _acceptor(_ioContext),
  // + 1 for 'this'
  _threadPool(_options._maxTcpSessions + 1) {}

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
    accept();
    _threadPool.push(shared_from_this());
  }
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' 
	 << ec.what() << " tcpPort=" << _options._tcpPort << '\n';
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
  _threadPoolHeartbeat.stop();
  _threadPool.stop();
}

void TcpAcceptor::run() {
  try {
    _ioContext.run();
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << '\n';
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
	char buffer[HSMSG_SIZE] = {};
	boost::asio::read(details->_socket, boost::asio::buffer(buffer, HSMSG_SIZE), ec);
	if (ec) {
	  CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << ec.what() << '\n';
	  return;
	}
	std::string_view received(buffer, HSMSG_SIZE);
	std::vector<std::string_view> fields;
	utility::split(received, fields, '|');
	char typeChar = fields[std::underlying_type_t<HSMSG_INDEX>(HSMSG_INDEX::TYPE)][0];
	std::string_view idView = fields[std::underlying_type_t<HSMSG_INDEX>(HSMSG_INDEX::ID)];
	std::string id(idView.data(), idView.size());
	SESSIONTYPE type = static_cast<SESSIONTYPE>(typeChar);
	for (auto it = _sessions.begin(); it != _sessions.end();)
	  if (!it->second.lock())
	    it = _sessions.erase(it);
	  else
	    ++it;
	switch (type) {
	case SESSIONTYPE::SESSION:
	  {
	    auto session = std::make_shared<TcpSession>(_options, details, shared_from_this());
	    _sessions[id] = session->weak_from_this();
	    session->start();
	    [[maybe_unused]] PROBLEMS problem = _threadPool.push(session);
	  }
	  break;
	case SESSIONTYPE::HEARTBEAT:
	  {
	    auto it = _sessions.find(id);
	    if (it == _sessions.end()) {
	      CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
		   << ":corresponding session not found" << std::endl;
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
	    [[maybe_unused]] PROBLEMS problem = _threadPoolHeartbeat.push(heartbeat);
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
  _threadPool.removeFromQueue(toRemove);
}

} // end of namespace tcp
