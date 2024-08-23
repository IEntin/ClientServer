/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpSession.h"

#include <array>

#include "Server.h"
#include "ServerOptions.h"
#include "Task.h"
#include "Tcp.h"

namespace tcp {

TcpSession::TcpSession(ServerWeakPtr server,
		       ConnectionPtr connection,
		       const CryptoPP::SecByteBlock& pubB) :
  RunnableT(ServerOptions::_maxTcpSessions),
  Session(server, pubB),
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
  boost::asio::post(_ioContext, [this] { readSize(); });
  return true;
}

void TcpSession::sendStatusToClient() {
  if (auto server = _server.lock(); server) {
    std::string clientIdStr;
    ioutility::toChars(_clientId, clientIdStr);
    unsigned size = clientIdStr.size();
    HEADER header
      { HEADERTYPE::CREATE_SESSION, size, size, COMPRESSORS::NONE, CRYPTO::NONE, DIAGNOSTICS::NONE, _status, _pubA.size() };
    Tcp::sendMsg(_socket, header, clientIdStr, _pubA);
  }
}

void TcpSession::run() noexcept {
  try {
    CountRunning countRunning;
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

void TcpSession::readSize() {
  std::memset(_sizeBuffer, '\0', ioutility::CONV_BUFFER_SIZE);
  boost::asio::async_read(_socket,
  boost::asio::buffer(_sizeBuffer),
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
      std::size_t payloadSz = 0;
      std::string_view sizeStr(_sizeBuffer, ioutility::CONV_BUFFER_SIZE);
      ioutility::fromChars(sizeStr, payloadSz);
      _request.resize(payloadSz);
      readRequest();
    });
}

void TcpSession::readRequest() {
  asyncWait();
  boost::asio::async_read(_socket,
    boost::asio::buffer(_request),
    [this] (const boost::system::error_code& ec, std::size_t transferred[[maybe_unused]]) mutable {
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
      if (processTask())
	sendReply();
      else {
	boost::asio::post(_ioContext, [this] {
	  _timeoutTimer.cancel();
	});
      }
    });
}

void TcpSession::write(const HEADER& header, std::string_view body) {
  serialize(header, _headerBuffer);
  std::size_t payloadSize = HEADER_SIZE + body.size();
  char sizeStr[ioutility::CONV_BUFFER_SIZE] = {};
  ioutility::toChars(payloadSize, sizeStr);
  std::array<boost::asio::const_buffer, 3> asioBuffers{ boost::asio::buffer(sizeStr),
						        boost::asio::buffer(_headerBuffer),
						        boost::asio::buffer(body) };
  boost::asio::async_write(_socket,
    asioBuffers,
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
	readSize();
    });
}

void TcpSession::asyncWait() {
  _timeoutTimer.expires_from_now(std::chrono::milliseconds(ServerOptions::_tcpTimeout));
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

void TcpSession::displayCapacityCheck(std::atomic<unsigned>& totalNumberObjects) const {
  Session::displayCapacityCheck(totalNumberObjects,
				getNumberObjects(),
				getNumberRunningByType(),
				_maxNumberRunningByType,
				_status);
}

} // end of namespace tcp
