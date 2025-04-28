/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpSession.h"

#include <array>

#include "Connection.h"
#include "Server.h"
#include "ServerOptions.h"
#include "Tcp.h"

namespace tcp {

static auto  TYPE{ "tcp" };

TcpSession::TcpSession(ServerWeakPtr server,
		       ConnectionPtr connection,
		       std::string_view msgHash,
		       const std::vector<unsigned char> pubBvector,
		       std::string_view signatureWithPubKey) :
  RunnableT(ServerOptions::_maxTcpSessions),
  Session(server, msgHash, pubBvector, signatureWithPubKey),
  _connection(std::move(connection)),
  _ioContext(_connection->_ioContext),
  _socket(std::move(_connection->_socket)),
  _timeoutTimer(_ioContext) {}

TcpSession::~TcpSession() {
  Info << '\n';
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
  auto lambda = [this] (
    const HEADER& header, std::string_view idStr, const CryptoPP::SecByteBlock& pubA) {
    Tcp::sendMsg(_socket, header, idStr, pubA);
  };
  Session::sendStatusToClient(lambda, _status);
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
  auto [header, payload] = buildReply(_status);
  if (payload.empty())
    return false;
  asyncWait();
  boost::asio::post(_ioContext, [this, header, payload] { write(header, payload); });
  return true;
}

void TcpSession::readRequest() {
  asyncWait();
  boost::asio::async_read_until(_socket,
    boost::asio::dynamic_buffer(_request),
    ENDOFMESSAGE,
    [this] (const boost::system::error_code& ec, std::size_t transferred) {
      if (_request.ends_with(ENDOFMESSAGE))
	_request.erase(transferred - ENDOFMESSAGESZ);
      if (auto self = weak_from_this().lock(); !self)
	return;
      if (ec) {
	switch (ec.value()) {
	case boost::asio::error::eof:
	case boost::asio::error::connection_reset:
	case boost::asio::error::broken_pipe:
	case boost::asio::error::connection_refused:
	  Debug << ec.what() << '\n';
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
       if (deserialize(_header, _request.data()))
	 _request.erase(0, HEADER_SIZE);
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

  void TcpSession::write(const HEADER& header, std::string_view payload) {
  char headerBuffer[HEADER_SIZE] = {};
  serialize(header, headerBuffer);
  std::array<boost::asio::const_buffer, 3> asioBuffers{ boost::asio::buffer(headerBuffer),
							boost::asio::buffer(payload),
							boost::asio::buffer(ENDOFMESSAGE) };
  boost::asio::async_write(_socket,
    asioBuffers,
    boost::asio::transfer_all(),
    [this](const boost::system::error_code& ec, std::size_t transferred[[maybe_unused]]) {
      if (auto self = weak_from_this().lock(); !self)
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
      _request.clear();
      boost::asio::post(_ioContext, [this] { readRequest(); });
    });
}

void TcpSession::asyncWait() {
  boost::system::error_code ec;
  _timeoutTimer.expires_from_now(
    boost::posix_time::milliseconds(ServerOptions::_tcpTimeout), ec);
  if (ec) {
    LogError << ec.what() << '\n';
    return;
  }
  _timeoutTimer.async_wait([this](const boost::system::error_code& ec) {
    if (auto self = weak_from_this().lock(); !self)
      return;
    if (ec) {
      switch (ec.value()) {
      case boost::asio::error::eof:
      case boost::asio::error::connection_reset:
      case boost::asio::error::broken_pipe:
      case boost::asio::error::connection_refused:
      case boost::asio::error::operation_aborted:
	Debug << ec.what() << '\n';
	break;
      default:
	LogError << ec.what() << '\n';
	break;
      }
    }
  });
}

void TcpSession::displayCapacityCheck(std::atomic<unsigned>& totalNumberObjects) const {
  Session::displayCapacityCheck(TYPE,
				totalNumberObjects,
				getNumberObjects(),
				getNumberRunningByType(),
				_maxNumberRunningByType,
				_status);
}

} // end of namespace tcp
