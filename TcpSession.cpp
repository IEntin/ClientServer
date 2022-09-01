/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpSession.h"
#include "Compression.h"
#include "MemoryPool.h"
#include "ServerOptions.h"
#include "ServerUtility.h"
#include "SessionDetails.h"
#include "TaskController.h"
#include "Utility.h"

namespace tcp {

TcpSession::TcpSession(const ServerOptions& options, SessionDetailsPtr details, RunnablePtr parent) :
  Runnable(parent, TaskController::instance(), parent, TCP, options._maxTcpSessions),
  _options(options),
  _details(details),
  _ioContext(details->_ioContext),
  _socket(details->_socket),
  _strand(boost::asio::make_strand(_ioContext)),
  _timer(_ioContext),
  _heartbeatTimer(_ioContext),
  // save for reference count
  _parent(parent) {
  _socket.set_option(boost::asio::socket_base::linger(false, 0));
  _socket.set_option(boost::asio::socket_base::reuse_address(true));
  _timer.expires_from_now(std::chrono::milliseconds(std::numeric_limits<int>::max()));
  _heartbeatTimer.expires_from_now(std::chrono::milliseconds(std::numeric_limits<int>::max()));
}

TcpSession::~TcpSession() {
  if (_socket.is_open()) {
    boost::system::error_code ignore;
    _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
    _socket.close(ignore);
  }
  if (_heartbeatThread.joinable())
    _heartbeatThread.join();
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool TcpSession::start() {
  const auto& local = _socket.local_endpoint();
  const auto& remote = _socket.remote_endpoint();
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
       << ":local " << local.address() << ':' << local.port()
       << ",remote " << remote.address() << ':' << remote.port() << std::endl;
  _problem.store(_typedSessions >= _max ? PROBLEMS::MAX_TCP_SESSIONS : PROBLEMS::NONE);
  char buffer[HEADER_SIZE] = {};
  encodeHeader(buffer, HEADERTYPE::REQUEST, 0, 0, COMPRESSORS::NONE, false, 0, _problem);
  boost::system::error_code ec;
  size_t result[[maybe_unused]] = boost::asio::write(_socket, boost::asio::buffer(buffer, HEADER_SIZE), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
    return false;
  }
  if (_options._tcpHeartbeatEnabled)
    _heartbeatThread = std::thread(&TcpSession::heartbeatThreadFunc, this);
  return true;
}

void TcpSession::run() noexcept {
  try {
    readHeader();
    if (_problem.load() == PROBLEMS::MAX_TCP_SESSIONS)
      _problem.store(PROBLEMS::NONE);
    _ioContext.run();
    if (_options._destroyBufferOnClientDisconnect)
      MemoryPool::destroyBuffers();
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << '\n';
  }
}

void TcpSession::stop() {}

bool TcpSession::onReceiveRequest() {
  _response.clear();
  _uncompressed.clear();
  bool bcompressed = isInputCompressed(_header);
  if (bcompressed) {
    static auto& printOnce[[maybe_unused]] =
      CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << " received compressed." << std::endl;
    if (!decompress(_request, _uncompressed)) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":decompression failed.\n";
      return false;
    }
  }
  else
    static auto& printOnce[[maybe_unused]] =
      CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << " received not compressed." << std::endl;
  TaskController::instance()->submitTask(_header, (bcompressed ? _uncompressed : _request), _response);
  if (!sendReply(_response))
    return false;
  return true;
}

bool TcpSession::sendReply(const Response& response) {
  std::string_view message = serverutility::buildReply(response, _options._compressor, 0);
  if (message.empty())
    return false;
  write(message);
  return true;
}

void TcpSession::readHeader() {
  asyncWait();
  std::memset(_headerBuffer, 0, HEADER_SIZE);
  auto weak = weak_from_this();
  boost::asio::async_read(_socket,
    boost::asio::buffer(_headerBuffer), boost::asio::bind_executor(_strand,
    [this, weak] (const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
      auto self = weak.lock();
      if (!self)
	return;
      if (_stopped || ec) {
	if (ec)
	  (ec == boost::asio::error::eof ? CLOG : CERR)
	    << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
	boost::system::error_code ignore;
	_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
	_socket.close(ignore);
	_timer.cancel(ignore);
	_heartbeatTimer.cancel(ignore);
	return;
      }
      _header = decodeHeader(std::string_view(_headerBuffer, HEADER_SIZE));
      if (!isOk(_header)) {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ": header is invalid.\n";
	return;
      }
      _request.clear();
      _request.resize(getCompressedSize(_header));
      readRequest();
    }));
}

void TcpSession::readRequest() {
  auto weak = weak_from_this();
  boost::asio::async_read(_socket,
    boost::asio::buffer(_request), boost::asio::bind_executor(_strand,
    [this, weak] (const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
      if (_stopped)
	return;
      boost::system::error_code err;
      size_t canceled = _timer.cancel(err);
      if (canceled == 0) {
	if (!_stopped)
	  CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << " timeout.\n";
	_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, err);
	_socket.close(err);
	_heartbeatTimer.cancel(err);
	return;
      }
      auto self = weak.lock();
      if (!self)
	return;
      if (ec) {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
	boost::system::error_code ignore;
	_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
	_socket.close(ignore);
	_heartbeatTimer.cancel(ignore);
	return;
      }
      onReceiveRequest();
    }));
}

void TcpSession::write(std::string_view reply) {
  asyncWait();
  auto weak = weak_from_this();
  boost::asio::async_write(_socket,
    boost::asio::buffer(reply.data(), reply.size()), boost::asio::bind_executor(_strand,
    [this, weak](const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
      if (_stopped)
	return;
      boost::system::error_code ignore;
      size_t canceled = _timer.cancel(ignore);
      if (canceled == 0) {
	if (!_stopped)
	  CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << " timeout.\n";
	return;
      }
      auto self = weak.lock();
      if (!self)
	return;
      if (ec) {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
	boost::system::error_code ignore;
	_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
	_socket.close(ignore);
	_heartbeatTimer.cancel(ignore);
	return;
      }
      readHeader();
    }));
}

void TcpSession::asyncWait() {
  if (_stopped)
    return;
  _timer.expires_from_now(std::chrono::milliseconds(_options._tcpTimeout));
  auto weak = weak_from_this();
  _timer.async_wait(boost::asio::bind_executor(_strand,
    [this, weak](const boost::system::error_code& ec) {
      if (_stopped)
	return;
      auto self = weak.lock();
      if (!self)
	return;
      if (ec && ec != boost::asio::error::operation_aborted)
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
   }));
}

void TcpSession::heartbeat() {
  if (_stopped)
    return;
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
  encodeHeader(_heartbeatBuffer, HEADERTYPE::HEARTBEAT, 0, 0, COMPRESSORS::NONE, false, 0);
  auto weak = weak_from_this();
    boost::asio::async_write(_socket, boost::asio::buffer(_heartbeatBuffer, HEADER_SIZE),
      boost::asio::bind_executor(_strand,
        [this, weak] (const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
	  if (_stopped)
	    return;
	  auto self = weak.lock();
	  if (!self)
	    return;
	  if (ec) {
	    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
	    return;
	  }
	  heartbeatWait();
	}));
}

void TcpSession::heartbeatWait() {
  if (_stopped)
    return;
  _heartbeatTimer.expires_from_now(std::chrono::milliseconds(_options._heartbeatPeriod));
  auto weak = weak_from_this();
  _heartbeatTimer.async_wait(boost::asio::bind_executor(_strand,
    [this, weak](const boost::system::error_code& ec) {
      if (_stopped)
	return;
      auto self = weak.lock();
      if (!self)
	return;
      if (ec) {
	CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
	return;
      }
      heartbeat();
    }));
}

void TcpSession::heartbeatThreadFunc() {
  try {
    heartbeatWait();
    _ioContext.run();
    if (_problem == PROBLEMS::MAX_TCP_SESSIONS && !_stopped)
      _removeFlag.store(true);
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << '\n';
  }
}

bool TcpSession::decompress(const std::vector<char>& input, std::vector<char>& uncompressed) {
  std::string_view received(input.data(), input.size());
  uncompressed.resize(getUncompressedSize(_header));
  Compression::uncompress(received, uncompressed);
  return true;
}

} // end of namespace tcp
