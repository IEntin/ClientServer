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
#include "ServerManager.h"
#include "ServerUtility.h"
#include "TaskController.h"
#include "Tcp.h"

namespace tcp {

TcpSession::TcpSession(const ServerOptions& options,
		       ConnectionDetailsPtr details,
		       std::string_view clientId,
		       ServerManager& serverManager) :
  RunnableT(options._maxTcpSessions),
  _options(options),
  _serverManager(serverManager),
  _clientId(clientId),
  _details(details),
  _ioContext(details->_ioContext),
  _socket(details->_socket),
  _strand(boost::asio::make_strand(_ioContext)),
  _timeoutTimer(_ioContext) {
  _status = _serverManager.incrementTotalSessions();
  _socket.set_option(boost::asio::socket_base::reuse_address(true));
  boost::asio::post(_ioContext, [this] { readHeader(); });
}

TcpSession::~TcpSession() {
  _serverManager.decrementTotalSessions();
  Trace << std::endl;
}

bool TcpSession::start() {
  checkCapacity();
  size_t size = _clientId.size();
  HEADER header{ HEADERTYPE::CREATE_SESSION, size, size, COMPRESSORS::NONE, false, _status };
  return sendMsg(_socket, header, _clientId).first;
}

void TcpSession::run() noexcept {
  _status.wait(STATUS::MAX_TOTAL_SESSIONS);
  try {
    _ioContext.run();
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
  }
  MemoryPool::destroyBuffers();
}

void TcpSession::stop() {
  STATUS expected = STATUS::MAX_TOTAL_SESSIONS;
  if (_status.compare_exchange_strong(expected, STATUS::NONE)) {
    _status = STATUS::NONE;
    _status.notify_one();
  }
  _ioContext.stop();
}

void TcpSession::notify() {
  STATUS expected = STATUS::MAX_TOTAL_SESSIONS;
  if (_status.compare_exchange_strong(expected, STATUS::NONE)) {
    _status = STATUS::NONE;
    _status.notify_one();
  }
}

void TcpSession::checkCapacity() {
  unsigned totalSessions = _serverManager.totalSessions();
  Info << "total sessions=" << totalSessions
       << " tcp sessions=" << _numberObjects << std::endl;
  if (_status == STATUS::MAX_TOTAL_SESSIONS) {
    Warn << "\nTotal clients=" << totalSessions
	 << " exceeds system capacity." << std::endl;
    return;
  }
  Runnable::checkCapacity();
  if (_status == STATUS::MAX_SPECIFIC_SESSIONS) {
    Warn << "\nThe number of tcp clients=" << _numberObjects
	 << " exceeds thread pool capacity." << std::endl;
  }
}

bool TcpSession::onReceiveRequest() {
  _uncompressed.clear();
  bool bcompressed = isCompressed(_header);
  if (bcompressed) {
    static auto& printOnce[[maybe_unused]] = Trace << " received compressed." << std::endl;
    size_t uncompressedSize = extractUncompressedSize(_header);
    _uncompressed.resize(uncompressedSize);
    if (!Compression::uncompress(_request, _request.size(), _uncompressed)) {
      LogError << "decompression failed." << std::endl;
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
      switch (_status) {
      case STATUS::MAX_SPECIFIC_SESSIONS:
      case STATUS::MAX_TOTAL_SESSIONS:
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
	LogError << ec.what() << std::endl;
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
	LogError << ec.what() << std::endl;
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
    LogError << ec.what() << std::endl;
    return;
  }
  _timeoutTimer.async_wait(boost::asio::bind_executor(_strand,
    [this, weakPtr](const boost::system::error_code& ec) {
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
    }));
}

} // end of namespace tcp
