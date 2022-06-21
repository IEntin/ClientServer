/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpConnection.h"
#include "Compression.h"
#include "ServerUtility.h"
#include "TaskController.h"
#include "TcpServer.h"
#include "Utility.h"
#include <iostream>

namespace tcp {

TcpConnection::TcpConnection(TaskControllerPtr taskController, int timeout, COMPRESSORS compressor, TcpServerPtr server) :
  _taskController(taskController),
  _ioContext(1),
  _socket(_ioContext),
  _timeout(timeout),
  _timer(_ioContext),
  _compressor(compressor),
  _server(server) {
  boost::system::error_code ignore;
  _socket.set_option(boost::asio::socket_base::linger(false, 0), ignore);
  _socket.set_option(boost::asio::socket_base::reuse_address(true), ignore);
}

TcpConnection::~TcpConnection() {
  boost::system::error_code ignore;
  _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
  _socket.close(ignore);
  _timer.cancel(ignore);
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool TcpConnection::stopped() const {
  return _server->stopped();
}

void TcpConnection::start() {
  const auto& local = _socket.local_endpoint();
  const auto& remote = _socket.remote_endpoint();
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
    << "-local " << local.address() << ':' << local.port()
    << ",remote " << remote.address() << ':' << remote.port()
    << std::endl;
  _server->pushToThreadPool(shared_from_this());
}

void TcpConnection::run() noexcept {
  readHeader();
  boost::system::error_code ec;
  _ioContext.run(ec);
  if (ec)
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << ':' << ec.what() << std::endl;
  MemoryPool::destroyBuffer();
}

bool TcpConnection::onReceiveRequest() {
  _response.clear();
  _uncompressed.clear();
  bool bcompressed = isInputCompressed(_header);
  if (bcompressed) {
    static auto& printOnce[[maybe_unused]] =
      CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
	<< " received compressed" << std::endl;
    if (!decompress(_request, _uncompressed)) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ":decompression failed" << std::endl;
      return false;
    }
  }
  else
    static auto& printOnce[[maybe_unused]] =
      CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
	<< " received not compressed" << std::endl;
  _taskController->submitTask(_header, (bcompressed ? _uncompressed : _request), _response);
  if (!sendReply(_response))
    return false;
  return true;
}

bool TcpConnection::sendReply(const Response& response) {
  std::string_view message =
    serverutility::buildReply(response, _compressor, _taskController->getMemoryPool());
  if (message.empty())
    return false;
  write(message);
  return true;
}

void TcpConnection::readHeader() {
  boost::system::error_code ignore;
  _timer.cancel(ignore);
  std::memset(_headerBuffer, 0, HEADER_SIZE);
  boost::asio::async_read(_socket,
			  boost::asio::buffer(_headerBuffer),
			  [this] (const boost::system::error_code& ec, size_t transferred) {
			    handleReadHeader(ec, transferred);
			  });
}

void TcpConnection::handleReadHeader(const boost::system::error_code& ec, [[maybe_unused]] size_t transferred) {
  if (stopped())
    return;
  asyncWait();
  if (!ec) {
    _header = decodeHeader(std::string_view(_headerBuffer, HEADER_SIZE));
    if (!isOk(_header)) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' <<__func__ << ':'
	   << "header is not valid" << std::endl;
      return;
    }
    _request.clear();
    _request.resize(getCompressedSize(_header));
    readRequest();
  }
  else {
    CERR << __FILE__ << ':' << __LINE__ << ' ' <<__func__ << ':'
	      << ec.what() << std::endl;
    boost::system::error_code ignore;
    _timer.cancel(ignore);
  }
}

void TcpConnection::readRequest() {
  boost::asio::async_read(_socket,
			  boost::asio::buffer(_request),
			  [this] (const boost::system::error_code& ec, size_t transferred) {
			    handleReadRequest(ec, transferred);
			  });
}

void TcpConnection::write(std::string_view reply) {
  boost::asio::async_write(_socket,
			   boost::asio::buffer(reply.data(), reply.size()),
			   [this](boost::system::error_code ec, size_t transferred) {
			     handleWriteReply(ec, transferred);
			   });
}

void TcpConnection::handleReadRequest(const boost::system::error_code& ec, [[maybe_unused]] size_t transferred) {
  if (stopped())
    return;
  boost::system::error_code ignore;
  _timer.cancel(ignore);
  if (!ec)
    onReceiveRequest();
  else {
    CERR << __FILE__ << ':' << __LINE__ << ' ' <<__func__ << ':' << ec.what() << std::endl;
    boost::system::error_code ignore;
    _timer.cancel(ignore);
  }
}

void TcpConnection::handleWriteReply(const boost::system::error_code& ec, [[maybe_unused]] size_t transferred) {
  if (stopped())
    return;
  boost::system::error_code ignore;
  _timer.cancel(ignore);
  if (!ec)
    readHeader();
  else {
    CERR << __FILE__ << ':' << __LINE__ << ' ' <<__func__ << ':'
	      << ec.what() << std::endl;
    boost::system::error_code ignore;
    _timer.cancel(ignore);
  }
}

void TcpConnection::asyncWait() {
  boost::system::error_code ignore;
  _timer.expires_from_now(std::chrono::seconds(_timeout), ignore);
  _timer.async_wait([this](const boost::system::error_code& err) {
		      if (err != boost::asio::error::operation_aborted) {
			CERR << __FILE__ << ':' << __LINE__ << ' '
                          << __func__ << ":timeout" << std::endl;
			boost::system::error_code ignore;
			_socket.close(ignore);
			_timer.expires_at(boost::asio::steady_timer::time_point::max(), ignore);
		      }
		    });
}

bool TcpConnection::decompress(const std::vector<char>& input, std::vector<char>& uncompressed) {
  std::string_view received(input.data(), input.size());
  uncompressed.resize(getUncompressedSize(_header));
  if (!Compression::uncompress(received, uncompressed)) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ":failed to uncompress payload" << std::endl;
    return false;
  }
  return true;
}

} // end of namespace tcp
