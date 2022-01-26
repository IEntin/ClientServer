/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpConnection.h"
#include "Task.h"
#include "CommUtility.h"
#include "ProgramOptions.h"
#include <iostream>

extern volatile std::atomic<bool> stopFlag;

namespace tcp {

TcpConnection::TcpConnection(boost::asio::io_context& io_context) : _socket(_ioContext), _timer(_ioContext) {}

TcpConnection::~TcpConnection() {
  boost::system::error_code ignore;
  _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
  _socket.close(ignore);
  _timer.cancel(ignore);
  if (_thread.joinable())
    _thread.detach();
}

void TcpConnection::start() {
  const auto& local = _socket.local_endpoint();
  const auto& remote = _socket.remote_endpoint();
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	    << "-local " << local.address() << ':' << local.port()
	    << ",remote " << remote.address() << ':' << remote.port()
	    << std::endl;
  _thread = std::thread(&TcpConnection::run, shared_from_this());
}

void TcpConnection::run() noexcept {
  readHeader();
  boost::system::error_code ec;
  _ioContext.run(ec);
  if (ec)
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ':' << ec.what() << std::endl;
}

bool TcpConnection::onReceiveRequest() {
  _response.clear();
  _uncompressed.clear();
  TaskContext context(_header);
  if (context.isInputCompressed()) {
    if (!context.decompress(_request, _uncompressed)) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ":decompression failed" << std::endl;
      return false;
    }
  }
  Task::process(context, (context.isInputCompressed() ? _uncompressed : _request), _response);
  if (!sendReply(_response))
    return false;
  return true;
}

bool TcpConnection::sendReply(Batch& batch) {
  std::string_view sendView = commutility::buildReply(batch);
  if (sendView.empty())
    return false;
  write(sendView);
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

void TcpConnection::handleReadHeader(const boost::system::error_code& ec, size_t transferred) {
  asyncWait();
  if (!(ec || stopFlag)) {
    _header = decodeHeader(std::string_view(_headerBuffer, HEADER_SIZE));
    size_t requestSize = std::get<static_cast<unsigned>(HEADER_INDEX::COMPRESSED_SIZE)>(_header);
    _request.clear();
    _request.resize(requestSize);
    readRequest();
  }
  else {
    if (ec)
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' <<__func__ << ':'
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

void TcpConnection::handleReadRequest(const boost::system::error_code& ec, size_t transferred) {
  boost::system::error_code ignore;
  _timer.cancel(ignore);
  if (!(ec || stopFlag))
    onReceiveRequest();
  else {
    if (ec)
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' <<__func__ << ':'
		<< ec.what() << std::endl;
    boost::system::error_code ignore;
    _timer.cancel(ignore);
  }
}

void TcpConnection::handleWriteReply(const boost::system::error_code& ec, size_t transferred) {
  boost::system::error_code ignore;
  _timer.cancel(ignore);
  if (!(ec || stopFlag))
    readHeader();
  else {
    if (ec)
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' <<__func__ << ':'
		<< ec.what() << std::endl;
    boost::system::error_code ignore;
    _timer.cancel(ignore);
  }
}

void TcpConnection::asyncWait() {
  const static unsigned timeout = ProgramOptions::get("Timeout", 1);
  boost::system::error_code ignore;
  _timer.expires_from_now(std::chrono::seconds(timeout), ignore);
  _timer.async_wait([this](const boost::system::error_code& err) {
		      if (err != boost::asio::error::operation_aborted) {
			std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":timeout" << std::endl;
			boost::system::error_code ignore;
			_socket.close(ignore);
			_timer.expires_at(boost::asio::steady_timer::time_point::max(), ignore);
		      }
		    });
}

} // end of namespace tcp
