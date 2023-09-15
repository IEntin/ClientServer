/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpSession.h"
#include "ServerOptions.h"
#include "ServerUtility.h"
#include "Tcp.h"
#include "Utility.h"

namespace tcp {

TcpSession::TcpSession() :
  RunnableT(ServerOptions::_maxTcpSessions, _displayType),
  _socket(_ioContext),
  _timeoutTimer(_ioContext) {}

TcpSession::~TcpSession() {
  Trace << '\n';
}

bool TcpSession::start() {
  _clientId = utility::getUniqueId();
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
  unsigned size = _clientId.size();
  HEADER header{ HEADERTYPE::CREATE_SESSION, size, size, COMPRESSORS::NONE, false, false, _status };
  auto ec = Tcp::sendMsg(_socket, header, _clientId);
  return !ec;
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
  std::string_view body = serverutility::buildReply(_response, header, _status);
  if (body.empty())
    return false;
  asyncWait();
  write(header, body);
  return true;
}

void TcpSession::readHeader() {
  asyncWait();
  boost::asio::async_read(_socket,
  boost::asio::buffer(_headerBuffer),
    [this] (const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
      auto weak = weak_from_this();
      auto self = weak.lock();
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
	bool berror = ec == boost::asio::error::eof ? false : true;
	if (berror)
	  LogError << ec.what() << '\n';
	else
	  Warn << ec.what() << '\n';
	_ioContext.stop();
	return;
      }
      HEADER header = decodeHeader(_headerBuffer);
      if (!isOk(header)) {
	LogError << "header is invalid." << '\n';
	return;
      }
      //_request.clear();
      _request.resize(extractPayloadSize(header));
      readRequest(header);
    });
}

void TcpSession::readRequest(const HEADER& header) {
  boost::asio::async_read(_socket,
    boost::asio::buffer(_request),
      [this, header] (const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
      auto weak = weak_from_this();
      auto self = weak.lock();
      if (!self)
	return;
      if (ec) {
	LogError << ec.what() << '\n';
	boost::asio::post(_ioContext, [this] {
	  _timeoutTimer.cancel();
	});
	return;
      }
      //_response.clear();
      if (serverutility::processRequest(header, _request, _response))
	sendReply();
    });
}

void TcpSession::write(const HEADER& header, std::string_view body) {
  _asioBuffers.clear();
  encodeHeader(_headerBuffer, header);
  _asioBuffers.emplace_back(boost::asio::buffer(_headerBuffer));
  _asioBuffers.emplace_back(boost::asio::buffer(body));
  boost::asio::async_write(_socket,
    _asioBuffers,
    [this](const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
      auto weak = weak_from_this();
      auto self = weak.lock();
      if (!self)
	return;
      if (ec) {
	LogError << ec.what() << '\n';
	boost::asio::post(_ioContext, [this] {
	  _timeoutTimer.cancel();
	});
	return;
      }
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
    auto weak = weak_from_this();
    auto self = weak.lock();
    if (!self)
      return;
    if (ec != boost::asio::error::operation_aborted) {
      if (ec) {
	LogError << ec.what() << '\n';
	_status = STATUS::TCP_PROBLEM;
	throw std::runtime_error(ec.what());
      }
      else {
	LogError << "timeout" << '\n';
	_status = STATUS::TCP_TIMEOUT;
 	throw std::runtime_error("timeout!");
     }
    }
  });
}

} // end of namespace tcp
