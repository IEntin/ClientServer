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
}

TcpSession::~TcpSession() {
  boost::system::error_code ignore;
  _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
  _socket.close(ignore);
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
  PROBLEMS problem = _typedSessions >= _max ? PROBLEMS::MAX_TCP_SESSIONS : PROBLEMS::NONE;
  char buffer[HEADER_SIZE] = {};
  encodeHeader(buffer, HEADERTYPE::REQUEST, 0, 0, COMPRESSORS::NONE, false, 0, problem);
  boost::system::error_code ec;
  size_t result[[maybe_unused]] = boost::asio::write(_socket, boost::asio::buffer(buffer, HEADER_SIZE), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
    return false;
  }
  if (_options._tcpHeartbeatEnabled)
    _heartbeatThread = std::thread(&TcpSession::heartbeatWait, this);
  return true;
}

void TcpSession::run() noexcept {
  try {
    readHeader();
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
  auto self = shared_from_this();
  boost::asio::async_read(_socket,
			  boost::asio::buffer(_headerBuffer), boost::asio::bind_executor(_strand,
			  [this, self] (const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
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
  auto self = shared_from_this();
  boost::asio::async_read(_socket,
			  boost::asio::buffer(_request), boost::asio::bind_executor(_strand,
			  [this, self] (const boost::system::error_code& ec, size_t transferred[[maybe_unused]]) {
			    if (_stopped || ec) {
			      if (ec)
				CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
			      boost::system::error_code ignore;
			      _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
			      _socket.close(ignore);
			      _timer.cancel(ignore);
			      _heartbeatTimer.cancel(ignore);
			      return;
			    }
			    onReceiveRequest();
			  }));
}

void TcpSession::write(std::string_view reply) {
  auto self = shared_from_this();
  boost::asio::async_write(_socket,
			   boost::asio::buffer(reply.data(), reply.size()), boost::asio::bind_executor(_strand,
			   [this, self](boost::system::error_code ec, size_t transferred[[maybe_unused]]) {
			     if (_stopped || ec) {
			       if (ec)
				 CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
			       boost::system::error_code ignore;
			       _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
			       _socket.close(ignore);
			       _timer.cancel(ignore);
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
  auto self = shared_from_this();
  _timer.async_wait(boost::asio::bind_executor(_strand,
    [this, self](const boost::system::error_code& err) {
      if (_stopped)
	return;
      if (err != boost::asio::error::operation_aborted)
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ": timeout.\n";
   }));
}

bool TcpSession::heartbeat() {
  if (_stopped)
    return false;
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '\n';
  asyncWait();
  encodeHeader(_heartbeatBuffer, HEADERTYPE::HEARTBEAT, 0, 0, COMPRESSORS::NONE, false, 0);
  auto self = shared_from_this();
  boost::system::error_code ec;
  _socket.wait(boost::asio::ip::tcp::socket::wait_write, ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
    return false;
  }
  boost::asio::async_write(_socket,
    boost::asio::buffer(_heartbeatBuffer, HEADER_SIZE),
    boost::asio::bind_executor(_strand,
      [this, self](boost::system::error_code ec, size_t transferred[[maybe_unused]])->auto {
	if (ec || _stopped) {
	  if (ec)
	    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
	  return false;
	}
	return true;
      }));
  return true;
}

void TcpSession::heartbeatWait() {
  auto self = shared_from_this();
  _heartbeatTimer.async_wait(boost::asio::bind_executor(_strand,
		     [this, self](const boost::system::error_code& err) {
		       if (!err) {
			 if (!heartbeat())
			   return;
			 _heartbeatTimer.expires_from_now(std::chrono::milliseconds(_options._heartbeatPeriod));
			 heartbeatWait();
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
