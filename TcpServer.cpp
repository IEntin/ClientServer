/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpServer.h"
#include "ServerOptions.h"
#include "SessionDetails.h"
#include "TaskController.h"
#include "TcpSession.h"
#include "Utility.h"

namespace tcp {

TcpServer::TcpServer(const ServerOptions& options) :
  Runnable(RunnablePtr(), TaskController::instance()),
  _options(options),
  _ioContext(1),
  _endpoint(boost::asio::ip::address_v4::any(), _options._tcpPort),
  _acceptor(_ioContext),
  // + 1 for 'this'
  _threadPool(_options._maxTcpSessions + 1) {}

TcpServer::~TcpServer() {
  boost::system::error_code ignore;
  _acceptor.close(ignore);
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool TcpServer::start() {
  boost::system::error_code ec;
  _acceptor.open(_endpoint.protocol(), ec);
  if (!ec)
    _acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
  if (!ec)
    _acceptor.set_option(boost::asio::socket_base::linger(false, 0), ec);
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
    _stopped.store(true);
    _threadPool.stop();
  }
  return !ec;
}

void TcpServer::stop() {
  _stopped.store(true);
  _ioContext.stop();
  _threadPool.stop();
}

void TcpServer::run() {
  try {
    _ioContext.run();
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << '\n';
  }
}

void TcpServer::accept() {
  auto details = std::make_shared<SessionDetails>();
  _acceptor.async_accept(details->_socket,
			 details->_endpoint,
			 [details, this](boost::system::error_code ec) {
			   if (ec)
			     (ec == boost::asio::error::operation_aborted ? CLOG : CERR)
			       << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
			   else {
			     char buffer[CLIENT_ID_SIZE] = {};
			     boost::asio::read(details->_socket, boost::asio::buffer(buffer, CLIENT_ID_SIZE), ec);
			     if (ec) {
			       CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << ec.what() << '\n';
			       return;
			     }
			     std::string_view received(buffer, CLIENT_ID_SIZE);
			     std::vector<std::string_view> fields;
			     utility::split(received, fields, '|');
			     char ch = fields[std::underlying_type_t<ID_INDEX>(ID_INDEX::SESSION)][0];
			     std::string_view idView = fields[std::underlying_type_t<ID_INDEX>(ID_INDEX::ID)];
			     SESSIONTYPE type = static_cast<SESSIONTYPE>(ch);
			     switch (type) {
			     case SESSIONTYPE::SESSION:
			       {
				 auto session = std::make_shared<TcpSession>(_options,
									     details,
									     idView,
									     shared_from_this());
				 auto [it, inserted] = _sessions.emplace(idView, session->weak_from_this());
				 if (!inserted) {
				   CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << "-not inserted\n";
				   return;
				 }
				 session->start();
				 [[maybe_unused]] PROBLEMS problem = _threadPool.push(session);
			       }
			       break;
			     case SESSIONTYPE::HEARTBEAT:
			       {
				  [[maybe_unused]] std::string id(idView.data(), idView.size());
			       }
			       break;
			     default:
			       break;
			     }
			     accept();
			   }
			 });
}

void TcpServer::remove(RunnablePtr runnable) {
  _threadPool.remove(runnable);
}

} // end of namespace tcp
