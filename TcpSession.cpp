/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpSession.h"
#include "ConnectionDetails.h"
#include "Server.h"
#include "ServerOptions.h"
#include "ServerUtility.h"
#include "Tcp.h"

namespace tcp {

TcpSession::TcpSession(const Server& server, ConnectionDetailsPtr details, std::string_view clientId) :
  RunnableT(server.getOptions()._maxTcpSessions, _name),
  _options(server.getOptions()),
  _clientId(clientId),
  _cryptoKeys(server.getKeys()),
   _details(details),
  _ioContext(details->_ioContext),
  _socket(std::move(details->_socket)),
  _timeoutTimer(_ioContext) {
  boost::system::error_code ec;
  _socket.set_option(boost::asio::socket_base::reuse_address(true), ec);
  if (ec) {
    LogError << ec.what() << std::endl;
    return;
  }
  boost::asio::post(_ioContext, [this] { readHeader(); });
}

TcpSession::~TcpSession() {
  Trace << std::endl;
}

bool TcpSession::sendStatusToClient() {
  size_t size = _clientId.size();
  HEADER header{ HEADERTYPE::CREATE_SESSION, size, size, COMPRESSORS::NONE, false, false, _status };
  return Tcp::sendMsg(_socket, header, _clientId).first;
}

void TcpSession::run() noexcept {
  CountRunning countRunning;
  try {
    _ioContext.run();
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
  }
}

void TcpSession::stop() {
  _ioContext.stop();
}

bool TcpSession::sendReply(const Response& response) {
  std::string_view body = serverutility::buildReply(_options, _cryptoKeys, response, _header, _status);
  if (body.empty())
    return false;
  asyncWait();
  write(body);
  return true;
}

void TcpSession::readHeader() {
  asyncWait();
  auto weakPtr = weak_from_this();
  boost::asio::async_read(_socket,
  boost::asio::buffer(_headerBuffer),
    [this, weakPtr] (const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
      auto self = weakPtr.lock();
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
	LOG_LEVEL level = ec == boost::asio::error::eof ? LOG_LEVEL::WARN : LOG_LEVEL::ERROR;
	Logger(level) << CODELOCATION << ':' << ec.what() << std::endl;
	_ioContext.stop();
	return;
      }
      _header = decodeHeader(_headerBuffer);
      if (!isOk(_header)) {
	LogError << "header is invalid." << std::endl;
	return;
      }
      _request.clear();
      _request.resize(extractCompressedSize(_header));
      readRequest();
    });
}

void TcpSession::readRequest() {
  auto weakPtr = weak_from_this();
  boost::asio::async_read(_socket,
    boost::asio::buffer(_request),
    [this, weakPtr] (const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
      auto self = weakPtr.lock();
      if (!self)
	return;
      if (ec) {
	LogError << ec.what() << std::endl;
	boost::asio::post(_ioContext, [this] {
	  _timeoutTimer.cancel();
	});
	return;
      }
      static thread_local Response response;
      response.clear();
      if (serverutility::processRequest(_cryptoKeys, _header, _request, response))
	sendReply(response);
    });
}

void TcpSession::write(std::string_view body) {
  auto weakPtr = weak_from_this();
  static thread_local std::vector<boost::asio::const_buffer> buffers;
  buffers.clear();
  encodeHeader(_headerBuffer, _header);
  buffers.emplace_back(boost::asio::buffer(_headerBuffer));
  buffers.emplace_back(boost::asio::buffer(body));
  boost::asio::async_write(_socket,
    buffers,
    [this, weakPtr](const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
      auto self = weakPtr.lock();
      if (!self)
	return;
      if (ec) {
	LogError << ec.what() << std::endl;
	boost::asio::post(_ioContext, [this] {
	  _timeoutTimer.cancel();
	});
	return;
      }
      readHeader();
    });
}

void TcpSession::asyncWait() {
  auto weakPtr = weak_from_this();
  boost::system::error_code ec;
  _timeoutTimer.expires_from_now(std::chrono::milliseconds(_options._tcpTimeout), ec);
  if (ec) {
    LogError << ec.what() << std::endl;
    return;
  }
  _timeoutTimer.async_wait([this, weakPtr](const boost::system::error_code& ec) {
    auto self = weakPtr.lock();
    if (!self)
      return;
    if (ec != boost::asio::error::operation_aborted) {
      if (ec)
	LogError << ec.what() << std::endl;
      else {
	LogError << "timeout" << std::endl;
	_status = STATUS::TCP_TIMEOUT;
      }
    }
  });
}

} // end of namespace tcp
