/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpConnection.h"
#include "Task.h"
#include "CommUtility.h"
#include "Compression.h"
#include "ProgramOptions.h"
#include <iostream>

namespace tcp {

const bool TcpConnection::_useStringView = ProgramOptions::get("StringTypeInTask", std::string()) == "STRINGVIEW";

TcpConnection::TcpConnection(boost::asio::io_context& io_context) : _socket(_ioContext), _timer(_ioContext) {}

TcpConnection::~TcpConnection() {
  boost::system::error_code ignore;
  _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
  _socket.close(ignore);
  _timer.cancel(ignore);
  if (_thread.joinable()) {
    _thread.join();
    std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ":thread joined" << std::endl;
  }
}

void TcpConnection::start() {
  const auto& local = _socket.local_endpoint();
  const auto& remote = _socket.remote_endpoint();
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	    << "-local " << local.address() << ':' << local.port()
	    << ",remote " << remote.address() << ':' << remote.port()
	    << std::endl;
  _thread = std::move(std::thread(&TcpConnection::run, this));
}

void TcpConnection::stop() {
  _ioContext.stop();
}

void TcpConnection::run() noexcept {
  readHeader();
  boost::system::error_code ec;
  _ioContext.run(ec);
  if (ec)
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ':' << ec.what() << std::endl;
  std::atomic_store(&_stopped, true);
}

bool TcpConnection::onReceiveRequest() {
  _response.clear();
  auto [uncomprSize, comprSize, compressor, done] =
    utility::decodeHeader(std::string_view(_header, HEADER_SIZE), true);
  bool bCompressed = compressor == LZ4;
  _uncompressed.clear();
  if (bCompressed) {
    if (!decompress(uncomprSize)) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ":decompression failed" << std::endl;
      return false;
    }
  }
  if (_useStringView)
    TaskSV::process((bCompressed ? _uncompressed : _request), _response);
  else {
    std::string_view input = bCompressed ?
      std::string_view(_uncompressed.data(), _uncompressed.size()) :
      std::string_view(_request.data(), _request.size());
    _requestBatch.clear();
    utility::split(input, _requestBatch);
    TaskST::process(_requestBatch, _response);
  }
  if (!sendReply(_response))
    return false;
  return true;
}

bool TcpConnection::decompress(size_t uncomprSize) {
  std::string_view received(_request.data(), _request.size());
  _uncompressed.resize(uncomprSize);
  if (!Compression::uncompress(received, _uncompressed)) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ":failed to uncompress payload" << std::endl;
    return false;
  }
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
  std::memset(_header, 0, HEADER_SIZE);
  auto self(shared_from_this());
  boost::asio::async_read(_socket,
			  boost::asio::buffer(_header),
			  [this, self] (const boost::system::error_code& ec, size_t transferred) {
			    handleReadHeader(ec, transferred);
			  });
}

void TcpConnection::handleReadHeader(const boost::system::error_code& ec, size_t transferred) {
  asyncWait();
  if (!ec && HEADER_SIZE == transferred) {
    HEADER t = utility::decodeHeader(std::string_view(_header, HEADER_SIZE), true);
    size_t requestSize = std::get<1>(t);
    _request.clear();
    _request.resize(requestSize);
    readRequest();
  }
  else {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' <<__func__ << ':'
	      << ec.what() << std::endl;
    boost::system::error_code ignore;
    _timer.cancel(ignore);
  }
}

void TcpConnection::readRequest() {
  auto self(shared_from_this());
  boost::asio::async_read(_socket,
			  boost::asio::buffer(_request),
			  [this, self] (const boost::system::error_code& ec, size_t transferred) {
			    handleReadRequest(ec, transferred);
			  });
}

void TcpConnection::write(std::string_view reply) {
  auto self(shared_from_this());
  boost::asio::async_write(_socket,
			   boost::asio::buffer(reply.data(), reply.size()),
			   [this, self](boost::system::error_code ec, size_t transferred) {
			     handleWriteReply(ec, transferred);
			   });
}

void TcpConnection::handleReadRequest(const boost::system::error_code& ec, size_t transferred) {
  boost::system::error_code ignore;
  _timer.cancel(ignore);
  if (!ec && _request.size() == transferred)
    onReceiveRequest();
  else {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' <<__func__ << ':'
	      << ec.what() << std::endl;
    boost::system::error_code ignore;
    _timer.cancel(ignore);
  }
}

void TcpConnection::handleWriteReply(const boost::system::error_code& ec, size_t transferred) {
  boost::system::error_code ignore;
  _timer.cancel(ignore);
  if (!ec)
    readHeader();
  else {
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
  auto self(shared_from_this());
  _timer.async_wait([this, self](const boost::system::error_code& err) {
		      if (err != boost::asio::error::operation_aborted) {
			std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":timeout" << std::endl;
			boost::system::error_code ignore;
			_timer.expires_at(boost::asio::steady_timer::time_point::max(), ignore);
		      }
		    });
}

} // end of namespace tcp
