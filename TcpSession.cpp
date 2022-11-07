/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpSession.h"
#include "TcpAcceptor.h"
#include "TcpHeartbeat.h"
#include "Compression.h"
#include "MemoryPool.h"
#include "ServerOptions.h"
#include "ServerUtility.h"
#include "SessionDetails.h"
#include "TaskController.h"
#include "Utility.h"

namespace tcp {

TcpSession::TcpSession(const ServerOptions& options, SessionDetailsPtr details, TcpAcceptorPtr parent) :
  Runnable(options._maxTcpSessions),
  _options(options),
  _details(details),
  _ioContext(details->_ioContext),
  _socket(details->_socket),
  _strand(boost::asio::make_strand(_ioContext)),
  _timeoutTimer(_ioContext),
  _parent(parent) {
  TaskController::totalSessions()++;
  _socket.set_option(boost::asio::socket_base::reuse_address(true));
  _timeoutTimer.expires_from_now(std::chrono::milliseconds(std::numeric_limits<int>::max()));
}

TcpSession::~TcpSession() {
  auto heartbeat = _heartbeat.lock();
  if (heartbeat)
    heartbeat->stop();
  TaskController::totalSessions()--;
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool TcpSession::start() {
  checkCapacity();
  char buffer[HEADER_SIZE] = {};
  encodeHeader(buffer, HEADERTYPE::SESSION, 0, 0, COMPRESSORS::NONE, false, _status);
  boost::system::error_code ec;
  size_t result[[maybe_unused]] = boost::asio::write(_socket, boost::asio::buffer(buffer, HEADER_SIZE), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
    return false;
  }
  return true;
}

void TcpSession::run() noexcept {
  try {
    readHeader();
    _ioContext.run();
    MemoryPool::destroyBuffers();
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << std::endl;
  }
}

void TcpSession::stop() {
  _ioContext.stop();
  _parent->remove(shared_from_this());
}

unsigned TcpSession::getNumberObjects() const {
  return _objectCounter._numberObjects;
}

void TcpSession::checkCapacity() {
  Runnable::checkCapacity();
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
       << " total sessions=" << TaskController::totalSessions() << ' '
       << "tcp sessions=" << _objectCounter._numberObjects << std::endl;
  if (_status == STATUS::MAX_NUMBER_RUNNABLES)
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << "\nThe number of tcp clients=" << _objectCounter._numberObjects
	 << " exceeds thread pool capacity." << std::endl;
}

bool TcpSession::onReceiveRequest() {
  _uncompressed.clear();
  bool bcompressed = isInputCompressed(_header);
  if (bcompressed) {
    static auto& printOnce[[maybe_unused]] =
      CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << " received compressed." << std::endl;
    size_t uncompressedSize = getUncompressedSize(_header);
    _uncompressed.resize(uncompressedSize);
    if (!Compression::uncompress(_request, _request.size(), _uncompressed)) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":decompression failed." << std::endl;
      return false;
    }
  }
  else
    static auto& printOnce[[maybe_unused]] =
      CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << " received not compressed." << std::endl;
  TaskController::instance()->processTask(_header, (bcompressed ? _uncompressed : _request), _response);
  if (!sendReply(_response))
    return false;
  return true;
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
  boost::asio::async_read(_socket,
  boost::asio::buffer(_headerBuffer), boost::asio::bind_executor(_strand,
    [this] (const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
      auto self = shared_from_this();
      if (_status == STATUS::MAX_NUMBER_RUNNABLES)
	_status.store(STATUS::NONE);
      if (ec) {
	(ec == boost::asio::error::eof ? CLOG : CERR)
	  << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
	_ioContext.stop();
	return;
      }
      _header = decodeHeader(_headerBuffer);
      if (!isOk(_header)) {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ": header is invalid." << std::endl;
	return;
      }
      _request.clear();
      _request.resize(getCompressedSize(_header));
      readRequest();
    }));
}

void TcpSession::readRequest() {
  boost::asio::async_read(_socket,
    boost::asio::buffer(_request), boost::asio::bind_executor(_strand,
    [this] (const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
      auto self = shared_from_this();
      if (ec) {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
	return;
      }
      onReceiveRequest();
    }));
}

void TcpSession::write(std::string_view msg, std::function<void(TcpSession*)> nextFunc) {
  boost::asio::async_write(_socket,
    boost::asio::buffer(msg.data(), msg.size()), boost::asio::bind_executor(_strand,
    [this, nextFunc](const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
      auto self = shared_from_this();
      if (ec) {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
	return;
      }
      if (nextFunc)
	nextFunc(this);
    }));
}

void TcpSession::asyncWait() {
  _timeoutTimer.expires_from_now(std::chrono::milliseconds(_options._tcpTimeout));
  _timeoutTimer.async_wait(boost::asio::bind_executor(_strand,
    [this](const boost::system::error_code& ec) {
      auto self = shared_from_this();
      if (ec != boost::asio::error::operation_aborted) {
	if (ec)
	  CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
	else {
	  CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ": timeout" << std::endl;
	  _status.store(STATUS::TCP_TIMEOUT);
	}
      }
    }));
}

} // end of namespace tcp
