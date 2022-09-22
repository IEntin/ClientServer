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
#include "TcpAcceptor.h"
#include "Utility.h"

namespace tcp {

TcpSession::TcpSession(const ServerOptions& options, SessionDetailsPtr details, TcpAcceptorPtr parent) :
  _options(options),
  _details(details),
  _ioContext(details->_ioContext),
  _socket(details->_socket),
  _strand(boost::asio::make_strand(_ioContext)),
  _timeoutTimer(_ioContext),
  _heartbeatPeriod(_options._heartbeatPeriod),
  _parent(parent) {
  TaskController::_totalSessions++;
  _socket.set_option(boost::asio::socket_base::reuse_address(true));
  _timeoutTimer.expires_from_now(std::chrono::milliseconds(std::numeric_limits<int>::max()));
}

TcpSession::~TcpSession() {
  TaskController::_totalSessions--;
  if (_socket.is_open()) {
    boost::system::error_code ignore;
    _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
    _socket.close(ignore);
  }
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool TcpSession::start() {
  const auto& local = _socket.local_endpoint();
  const auto& remote = _socket.remote_endpoint();
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
       << ":local " << local.address() << ':' << local.port()
       << ",remote " << remote.address() << ':' << remote.port() << std::endl;
  _problem.store(_objectCounter._numberObjects > _options._maxTcpSessions ?
		 PROBLEMS::MAX_TCP_SESSIONS : PROBLEMS::NONE);
  char buffer[HEADER_SIZE] = {};
  encodeHeader(buffer, HEADERTYPE::REQUEST, 0, 0, COMPRESSORS::NONE, false, 0, _problem);
  boost::system::error_code ec;
  size_t result[[maybe_unused]] = boost::asio::write(_socket, boost::asio::buffer(buffer, HEADER_SIZE), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
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
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << '\n';
  }
}

unsigned TcpSession::getNumberObjects() const {
  return _objectCounter._numberObjects;
}

PROBLEMS TcpSession::checkCapacity() const {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
       << " total sessions=" << TaskController::_totalSessions << ' '
       << "tcp sessions=" << _objectCounter._numberObjects << std::endl;
  if (_objectCounter._numberObjects > _options._maxTcpSessions) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << "\nThe number of tcp clients=" << _objectCounter._numberObjects
	 << " at thread pool capacity.\n"
	 << "This client will wait in the queue.\n"
	 << "Close one of running tcp clients\n"
	 << "or increase \"MaxTcpSessions\" in ServerOptions.json.\n"
	 << "You can also close this client and try again later.\n";
    return PROBLEMS::MAX_TCP_SESSIONS;
  }
  return PROBLEMS::NONE;
}

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
  asyncWait();
  write(message, &TcpSession::readHeader);
  return true;
}

void TcpSession::readHeader() {
  asyncWait();
  std::memset(_headerBuffer, 0, HEADER_SIZE);
  boost::system::error_code ignore;
  _socket.wait(boost::asio::ip::tcp::socket::wait_read, ignore);
  boost::asio::async_read(_socket,
    boost::asio::buffer(_headerBuffer), boost::asio::bind_executor(_strand,
    [this] (const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
      if (_problem == PROBLEMS::MAX_TCP_SESSIONS)
	_problem.store(PROBLEMS::NONE);
      if (_parent->stopped()) {
	_ioContext.stop();
	return;
      }
      if (ec) {
	(ec == boost::asio::error::eof ? CLOG : CERR)
	  << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
	_ioContext.stop();
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
  boost::asio::async_read(_socket,
    boost::asio::buffer(_request), boost::asio::bind_executor(_strand,
    [this] (const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
      if (_parent->stopped()) {
	_ioContext.stop();
	return;
      }
      if (ec) {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
	_ioContext.stop();
	return;
      }
      onReceiveRequest();
    }));
}

void TcpSession::write(std::string_view msg, std::function<void(TcpSession*)> nextFunc) {
  boost::asio::async_write(_socket,
    boost::asio::buffer(msg.data(), msg.size()), boost::asio::bind_executor(_strand,
    [this, nextFunc](const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
      if (_parent->stopped()) {
	_ioContext.stop();
	return;
      }
      if (ec) {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
	_heartbeatPeriod = 1;
	_ioContext.stop();
	return;
      }
      if (nextFunc)
	nextFunc(this);
    }));
}

void TcpSession::asyncWait() {
  if (_parent->stopped()) {
    _ioContext.stop();
    return;
  }
  _timeoutTimer.expires_from_now(std::chrono::milliseconds(_options._tcpTimeout));
  _timeoutTimer.async_wait(boost::asio::bind_executor(_strand,
    [this](const boost::system::error_code& ec) {
      if (_parent->stopped()) {
	_ioContext.stop();
	return;
      }
      if (ec != boost::asio::error::operation_aborted) {
	if (ec)
	  CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
	else
	  CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ": timeout\n";
	_ioContext.stop();
      }
   }));
}

bool TcpSession::decompress(const std::vector<char>& input, std::vector<char>& uncompressed) {
  std::string_view received(input.data(), input.size());
  uncompressed.resize(getUncompressedSize(_header));
  Compression::uncompress(received, uncompressed);
  return true;
}

} // end of namespace tcp
