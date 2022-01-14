/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "AsioServer.h"
#include "Compression.h"
#include "ProgramOptions.h"
#include <iostream>

extern volatile std::atomic<bool> stopFlag;

Session::Session(boost::asio::ip::tcp::socket socket)
  : _socket(std::move(socket)), _timer(_socket.get_executor()) {}

void Session::start() {
  readHeader();
}

void Session::readHeader() {
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

void Session::handleReadHeader(const boost::system::error_code& ec, size_t transferred) {
  asyncWait();
  if (!ec && HEADER_SIZE == transferred) {
    HEADER t = utility::decodeHeader(std::string_view(_header, HEADER_SIZE), true);
    size_t requestSize = std::get<1>(t);
    _request.resize(requestSize);
    readRequest();
  }
  else
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' <<__func__ << ':'
	      << std::strerror(errno) << std::endl;
}

void Session::readRequest() {
  auto self(shared_from_this());
  boost::asio::async_read(_socket,
			  boost::asio::buffer(_request),
			  [this, self] (const boost::system::error_code& ec, size_t transferred) {
			    handleReadRequest(ec, transferred);
			  });
}

void Session::writeReply() {
  auto self(shared_from_this());
  std::vector<boost::asio::const_buffer> buffers;
  utility::encodeHeader(_header, _request.size(), _request.size(), EMPTY_COMPRESSOR);
  buffers.push_back(boost::asio::buffer(_header, HEADER_SIZE));
  buffers.push_back(boost::asio::buffer(_request, _request.size()));
  boost::asio::async_write(_socket,
			   buffers,
			   [this, self](boost::system::error_code ec, size_t transferred) {
			     handleWriteReply(ec, transferred);
			   });
}

void Session::handleReadRequest(const boost::system::error_code& ec, size_t transferred) {
  boost::system::error_code ignore;
  _timer.cancel(ignore);
  if (!ec && _request.size() == transferred) {
    writeReply();
  }
  else
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' <<__func__ << ':'
	      << std::strerror(errno) << std::endl;
}

void Session::handleWriteReply(const boost::system::error_code& ec, size_t transferred) {
  boost::system::error_code ignore;
  _timer.cancel(ignore);
  if (!ec) {
    if (!stopFlag)
      readHeader();
  }
  else
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' <<__func__ << ':'
	      << std::strerror(errno) << std::endl;
}

void Session::asyncWait() {
  const static unsigned timeout = ProgramOptions::get("Timeout", 1);
  boost::system::error_code ignore;
  _timer.expires_from_now(std::chrono::seconds(timeout), ignore);
  auto self(shared_from_this());
  static const std::string function(__FUNCTION__);
  _timer.async_wait([this, self](const boost::system::error_code& err) {
		      if (err != boost::asio::error::operation_aborted) {
			std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":timeout" << std::endl;
			//boost::system::error_code ignore;
			//_socket.close(ignore);
			//_timer.expires_at(boost::asio::steady_timer::time_point::max(), ignore);
		      }
		    });
}

boost::asio::io_context AsioServer::_ioContext;
std::thread AsioServer::_thread;
std::list<AsioServer> AsioServer::_servers;

AsioServer::AsioServer(boost::asio::io_context& ioContext,
		       const boost::asio::ip::tcp::endpoint& endpoint)
  : _acceptor(ioContext, endpoint) {
  accept();
}

void AsioServer::accept() {
  _acceptor.async_accept([this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
			   if (!ec)
			     std::make_shared<Session>(std::move(socket))->start();
			   accept();
			 });
}

void AsioServer::run() {
  _ioContext.run();
}

bool AsioServer::startServers() {
  try {
    std::string tcpPortsStr = ProgramOptions::get("TcpPorts", std::string());
    std::vector<std::string> tcpPortsVector;
    utility::split(tcpPortsStr, tcpPortsVector, ",\n ");
    for (auto port : tcpPortsVector) {
      boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), std::atoi(port.c_str()));
      _servers.emplace_back(_ioContext, endpoint);
    }
    std::thread tmp(AsioServer::run);
    _thread.swap(tmp);
  }
  catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }
  return true;
}

void  AsioServer::joinThread() {
  _ioContext.stop();
  if (_thread.joinable()) {
    _thread.join();
    std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << " ... _thread joined ..." << std::endl;
  }
}
