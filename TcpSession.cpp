/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpSession.h"

#include <array>

#include "IOUtility.h"
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
  boost::asio::post(_ioContext, [this] { readRequest(); });
  return true;
}

void TcpSession::sendStatusToClient() {
  if (auto server = _server.lock(); server) {
    std::string clientIdStr;
    ioutility::toChars(_clientId, clientIdStr);
    unsigned size = clientIdStr.size();
    const auto& pubA(_crypto->getPubKey());
    HEADER header
      { HEADERTYPE::DH_HANDSHAKE, size, COMPRESSORS::NONE, DIAGNOSTICS::NONE, _status, pubA.size() };
    Tcp::sendMsg(_socket, header, clientIdStr, pubA);
  }
}

void TcpSession::run() noexcept {
  try {
    switch (_status) {
    case STATUS::MAX_OBJECTS_OF_TYPE:
    case STATUS::MAX_TOTAL_OBJECTS:
      _status = STATUS::NONE;
      break;
    default:
      break;
    }
    CountRunning countRunning;
    _ioContext.run();
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }
}

void TcpSession::stop() {
  _stopped = true;
  _ioContext.stop();
}

bool TcpSession::sendReply() {
  std::string_view payload = buildReply(_status);
  if (payload.empty())
    return false;
  asyncWait();
  boost::asio::post(_ioContext, [this, payload] { write(payload); });
  return true;
}

void TcpSession::readRequest() {
  asyncWait();
  boost::asio::async_read_until(_socket,
    boost::asio::dynamic_string_buffer(_request),
    utility::ENDOFMESSAGE,
    [this] (const boost::system::error_code& ec, std::size_t transferred) {
      if (transferred > utility::ENDOFMESSAGE.size())
	if (_request.ends_with(utility::ENDOFMESSAGE))
	  _request.erase(transferred - utility::ENDOFMESSAGE.size());
      auto self = weak_from_this().lock();
      if (!self)
	return;
      if (ec) {
	switch (ec.value()) {
	case boost::asio::error::eof:
	case boost::asio::error::connection_reset:
	case boost::asio::error::broken_pipe:
	case boost::asio::error::connection_refused:
	  Info << ec.what() << '\n';
	  break;
	default:
	  Warn << ec.what() << '\n';
	  break;
	}
	boost::asio::post(_ioContext, [this] {
	  _timeoutTimer.cancel();
	});
	return;
      }
      if (processTask())
	boost::asio::post(_ioContext, [this] { sendReply(); });
      else {
	boost::asio::post(_ioContext, [this] {
	  _timeoutTimer.cancel();
	});
      }
    });
  std::size_t numberCanceled = _timeoutTimer.cancel();
  if (numberCanceled == 0) {
    LogError << "timeout\n";
    _status = STATUS::TCP_TIMEOUT;
  }
}

void TcpSession::write(std::string_view payload) {
  HEADER header
    { HEADERTYPE::SESSION, payload.size(), ServerOptions::_compressor, DIAGNOSTICS::NONE, _status, 0 };
  char headerBuffer[HEADER_SIZE] = {};
  serialize(header, headerBuffer);
  std::array<boost::asio::const_buffer, 2> asioBuffers{ boost::asio::buffer(headerBuffer),
							boost::asio::buffer(payload) };
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
      std::size_t numberCanceled = _timeoutTimer.cancel();
      if (numberCanceled == 0) {
	LogError << "timeout\n";
	_status = STATUS::TCP_TIMEOUT;
	return;
      }
      _request.erase(0);
      boost::asio::post(_ioContext, [this] { readRequest(); });
    });
}

void TcpSession::asyncWait() {
  _timeoutTimer.expires_from_now(std::chrono::milliseconds(ServerOptions::_tcpTimeout));
  _timeoutTimer.async_wait([this](const boost::system::error_code& ec) {
    auto self = weak_from_this().lock();
    if (!self)
      return;
    if (ec) {
      switch (ec.value()) {
      case boost::asio::error::eof:
      case boost::asio::error::connection_reset:
      case boost::asio::error::broken_pipe:
      case boost::asio::error::connection_refused:
	Info << ec.what() << '\n';
	break;
      case boost::asio::error::operation_aborted:
	break;
      default:
	LogError << ec.what() << '\n';
	break;
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
