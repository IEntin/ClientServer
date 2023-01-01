/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpSession.h"
#include "Compression.h"
#include "ConnectionDetails.h"
#include "Globals.h"
#include "Logger.h"
#include "MemoryPool.h"
#include "ServerOptions.h"
#include "ServerUtility.h"
#include "TaskController.h"
#include "Tcp.h"

namespace tcp {

TcpSession::TcpSession(const ServerOptions& options,
		       ConnectionDetailsPtr details,
		       std::string_view clientId) :
  RunnableT(options._maxTcpSessions),
  _options(options),
  _clientId(clientId),
  _details(details),
  _ioContext(details->_ioContext),
  _socket(details->_socket),
  _strand(boost::asio::make_strand(_ioContext)),
  _timeoutTimer(_ioContext) {
  TaskController::totalSessions()++;
  _socket.set_option(boost::asio::socket_base::reuse_address(true));
  boost::asio::post(_ioContext, [this] { readHeader(); });
}

TcpSession::~TcpSession() {
  TaskController::totalSessions()--;
  Trace << std::endl;
}

bool TcpSession::start() {
  checkCapacity();
  size_t size = _clientId.size();
  HEADER header{ HEADERTYPE::CREATE_SESSION, size, size, COMPRESSORS::NONE, false, _status };
  return sendMsg(_socket, header, _clientId).first;
}

void TcpSession::run() noexcept {
  CountRunning countRunning;
  try {
    _ioContext.run();
  }
  catch (const std::exception& e) {
    LogError << ' ' << e.what() << std::endl;
  }
  MemoryPool::destroyBuffers();
}

void TcpSession::stop() {
  _ioContext.stop();
}

void TcpSession::checkCapacity() {
  Runnable::checkCapacity();
  Info << " total sessions=" << TaskController::totalSessions() << " tcp sessions="
       << _numberObjects << ",running=" << _numberRunning << std::endl;
  if (_status == STATUS::MAX_SPECIFIC_SESSIONS)
    Warn << "\nThe number of running tcp clients=" << _numberRunning
	 << " is at thread pool capacity." << std::endl;
}

bool TcpSession::onReceiveRequest() {
  _uncompressed.clear();
  bool bcompressed = isCompressed(_header);
  if (bcompressed) {
    static auto& printOnce[[maybe_unused]] = Trace << " received compressed." << std::endl;
    size_t uncompressedSize = extractUncompressedSize(_header);
    _uncompressed.resize(uncompressedSize);
    if (!Compression::uncompress(_request, _request.size(), _uncompressed)) {
      LogError << ":decompression failed." << std::endl;
      return false;
    }
  }
  else
    static auto& printOnce[[maybe_unused]] = Trace << " received not compressed." << std::endl;
  static thread_local Response response;
  response.clear();
  auto weakPtr = TaskController::weakInstance();
  auto taskController = weakPtr.lock();
  if (taskController)
    taskController->processTask(_header, (bcompressed ? _uncompressed : _request), response);
  else
    return false;
  return sendReply(response);
}

bool TcpSession::sendReply(const Response& response) {
  std::string_view message = serverutility::buildReply(response, _options._compressor, _status);
  if (message.empty())
    return false;
  asyncWait();
  write(message, &TcpSession::readHeader);
  return true;
}

void TcpSession::readHeader() {
  asyncWait();
  auto weakPtr = weak_from_this();
  boost::asio::async_read(_socket,
  boost::asio::buffer(_headerBuffer), boost::asio::bind_executor(_strand,
    [this, weakPtr] (const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
      auto self = weakPtr.lock();
      if (!self)
	return;
      if (_status == STATUS::MAX_SPECIFIC_SESSIONS)
	_status = STATUS::NONE;
      if (ec) {
	LOG_LEVEL level = ec == boost::asio::error::eof ? LOG_LEVEL::WARN : LOG_LEVEL::ERROR;
	Logger(level) << CODELOCATION << ':' << ec.what() << std::endl;
	_ioContext.stop();
	return;
      }
      _header = decodeHeader(_headerBuffer);
      if (!isOk(_header)) {
	LogError << ": header is invalid." << std::endl;
	return;
      }
      _request.clear();
      _request.resize(extractCompressedSize(_header));
      readRequest();
    }));
}

void TcpSession::readRequest() {
  auto weakPtr = weak_from_this();
  boost::asio::async_read(_socket,
    boost::asio::buffer(_request), boost::asio::bind_executor(_strand,
    [this, weakPtr] (const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
      auto self = weakPtr.lock();
      if (!self)
	return;
      if (ec) {
	LogError << ':' << ec.what() << std::endl;
	return;
      }
      onReceiveRequest();
    }));
}

void TcpSession::write(std::string_view msg, std::function<void(TcpSession*)> nextFunc) {
  auto weakPtr = weak_from_this();
  boost::asio::async_write(_socket,
    boost::asio::buffer(msg.data(), msg.size()), boost::asio::bind_executor(_strand,
    [this, weakPtr, nextFunc](const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
      auto self = weakPtr.lock();
      if (!self)
	return;
      if (ec) {
	LogError << ':' << ec.what() << std::endl;
	return;
      }
      if (nextFunc)
	nextFunc(this);
    }));
}

void TcpSession::asyncWait() {
  auto weakPtr = weak_from_this();
  boost::system::error_code ec;
  _timeoutTimer.expires_from_now(std::chrono::milliseconds(_options._tcpTimeout), ec);
  if (ec) {
    LogError << ':' << ec.what() << std::endl;
    return;
  }
  _timeoutTimer.async_wait(boost::asio::bind_executor(_strand,
    [this, weakPtr](const boost::system::error_code& ec) {
      auto self = weakPtr.lock();
      if (!self)
	return;
      if (ec != boost::asio::error::operation_aborted) {
	if (ec)
	  LogError << ':' << ec.what() << std::endl;
	else {
	  LogError << ": timeout" << std::endl;
	  _status = STATUS::TCP_TIMEOUT;
	}
      }
    }));
}

} // end of namespace tcp
