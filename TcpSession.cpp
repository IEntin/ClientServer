/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpSession.h"

#include "Connection.h"
#include "DHKeyExchange.h"
#include "Server.h"
#include "ServerOptions.h"
#include "Task.h"
#include "Tcp.h"

namespace tcp {

TcpSession::TcpSession(Server& server, ConnectionPtr connection) :
  RunnableT(ServerOptions::_maxTcpSessions),
  Session(server),
  _connection(std::move(connection)),
  _ioContext(_connection->_ioContext),
  _socket(std::move(_connection->_socket)),
  _timeoutTimer(_ioContext) {}

TcpSession::~TcpSession() {
  Trace << '\n';
}

bool TcpSession::start() {
  boost::system::error_code ec;
  _socket.set_option(boost::asio::socket_base::reuse_address(true), ec);
  if (ec) {
    LogError << ec.what() << '\n';
    return false;
  }
  Info << _socket.local_endpoint() << ' ' << _socket.remote_endpoint() << '\n';
  boost::asio::post(_ioContext, [this] { readHeader(); });
  return true;
}

bool TcpSession::sendStatusToClient() {
  struct DoAtExit {
    DoAtExit(Server& server) : _server(server) {}
    ~DoAtExit() {
      _server.setStarted();
    }
    Server& _server;
  } doAtExit(_server);
  std::string payload;
  ioutility::toChars(_clientId, payload);
  payload.append(_Astring);
  unsigned size = payload.size();
  HEADER header
    { HEADERTYPE::CREATE_SESSION, size, size, COMPRESSORS::NONE, false, false, _status, _Astring.size() };
  auto ec = Tcp::sendMsg(_socket, header, payload);
  if (!ec) {
    std::string Bstring;
    ec = Tcp::readMsg(_socket, header, Bstring);
    if (ec)
      return false;
    CryptoPP::Integer crossPub(Bstring.data());
    _key = DHKeyExchange::step2(_priv, crossPub);
  }
  else {
    LogError << ec.what() << '\n';
    return false;
  }
  return true;
}

void TcpSession::run() noexcept {
  CountRunning countRunning;
  try {
    _ioContext.run();
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }
}

void TcpSession::stop() {
  _ioContext.stop();
}

bool TcpSession::sendReply() {
  HEADER header;
  std::string_view body = buildReply(header, _status);
  if (body.empty())
    return false;
  asyncWait();
  write(header, body);
  return true;
}

void TcpSession::readHeader() {
  boost::asio::async_read(_socket,
  boost::asio::buffer(_headerBuffer),
    [this] (const boost::system::error_code& ec, std::size_t transferred[[maybe_unused]]) {
      auto self = weak_from_this().lock();
      if (!self)
	return;
      switch (_status) {
      case STATUS::MAX_OBJECTS_OF_TYPE:
      case STATUS::MAX_TOTAL_OBJECTS:
	_status = STATUS::NONE;
	break;
      default:
	break;
      }
      if (ec) {
	(ec == boost::asio::error::eof ? Warn : LogError) << ec.what() << '\n';
	_ioContext.stop();
	return;
      }
      HEADER header = decodeHeader(_headerBuffer);
      if (!isOk(header)) {
	LogError << "header is invalid." << '\n';
	return;
      }
      _request.resize(extractPayloadSize(header));
      readRequest(header);
    });
}

void TcpSession::readRequest(HEADER& header) {
  asyncWait();
  boost::asio::async_read(_socket,
    boost::asio::buffer(_request),
    [this, header] (const boost::system::error_code& ec, std::size_t transferred[[maybe_unused]]) mutable {
      auto self = weak_from_this().lock();
      if (!self)
	return;
      if (ec) {
	LogError << ec.what() << '\n';
	boost::asio::post(_ioContext, [this] {
	  _timeoutTimer.cancel();
	});
	return;
      }
      if (_status != STATUS::NONE)
	return;
      if (processTask(header))
	sendReply();
      else {
	boost::asio::post(_ioContext, [this] {
	  _timeoutTimer.cancel();
	});
      }
    });
}

void TcpSession::write(const HEADER& header, std::string_view body) {
  _asioBuffers.clear();
  encodeHeader(_headerBuffer, header);
  _asioBuffers.emplace_back(boost::asio::buffer(_headerBuffer));
  _asioBuffers.emplace_back(boost::asio::buffer(body));
  boost::asio::async_write(_socket,
    _asioBuffers,
    [this](const boost::system::error_code& ec, std::size_t transferred[[maybe_unused]]) {
      auto self = weak_from_this().lock();
      if (!self)
	return;
      if (ec) {
	LogError << ec.what() << '\n';
	boost::asio::post(_ioContext, [this] {
	  _timeoutTimer.cancel();
	});
	return;
      }
      if (_status == STATUS::NONE)
	readHeader();
    });
}

void TcpSession::asyncWait() {
  boost::system::error_code ec;
  _timeoutTimer.expires_from_now(std::chrono::milliseconds(ServerOptions::_tcpTimeout), ec);
  if (ec) {
    LogError << ec.what() << '\n';
    return;
  }
  _timeoutTimer.async_wait([this](const boost::system::error_code& ec) {
    auto self = weak_from_this().lock();
    if (!self)
      return;
    if (ec != boost::asio::error::operation_aborted) {
      if (ec) {
	LogError << ec.what() << '\n';
	_status = STATUS::TCP_PROBLEM;
      }
      else {
	LogError << "timeout" << '\n';
	_status = STATUS::TCP_TIMEOUT;
      }
    }
  });
}

} // end of namespace tcp
