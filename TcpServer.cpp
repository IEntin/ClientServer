/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpServer.h"
#include "Task.h"
#include "CommUtility.h"
#include "Compression.h"
#include "ProgramOptions.h"
#include <iostream>

extern volatile std::atomic<bool> stopFlag;

const bool Session::_useStringView = ProgramOptions::get("StringTypeInTask", std::string()) == "STRINGVIEW";

Session::Session(const std::string& port, boost::asio::ip::tcp::socket socket)
  : _port(port), _socket(std::move(socket)), _timer(_socket.get_executor()) {}

void Session::start() {
  readHeader();
}

bool Session::onReceiveRequest() {
  Batch response;
  auto [uncomprSize, comprSize, compressor, done] =
    utility::decodeHeader(std::string_view(_header, HEADER_SIZE), true);
  bool bCompressed = compressor == LZ4;
  static std::vector<char> uncompressed;
  uncompressed.clear();
  if (bCompressed) {
    if (!decompress(uncomprSize, uncompressed)) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ":decompression failed" << std::endl;
      return false;
    }
  }
  if (_useStringView)
    TaskSV::process(_port, (bCompressed ? uncompressed : _request), response);
  else {
    static Batch batch;
    batch.clear();
    std::string_view input = bCompressed ?
      std::string_view(uncompressed.data(), uncompressed.size()) :
      std::string_view(_request.data(), _request.size());
    utility::split(input, batch);
    TaskST::process(_port, batch, response);
  }
  if (!sendReply(response))
    return false;
  return true;
}

bool Session::decompress(size_t uncomprSize, std::vector<char>& uncompressed) {
  std::string_view received(_request.data(), _request.size());
  uncompressed.resize(uncomprSize);
  if (!Compression::uncompress(received, uncompressed)) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ":failed to uncompress payload" << std::endl;
    return false;
  }
  return true;
}

bool Session::sendReply(Batch& batch) {
  std::string_view sendView = commutility::buildReply(batch);
  if (sendView.empty())
    return false;
  write(sendView);
  return true;
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
    _request.clear();
    _request.resize(requestSize);
    readRequest();
  }
  else {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' <<__func__ << ':'
	      << std::strerror(errno) << std::endl;
    boost::system::error_code ignore;
    _timer.cancel(ignore);
  }
}

void Session::readRequest() {
  auto self(shared_from_this());
  boost::asio::async_read(_socket,
			  boost::asio::buffer(_request),
			  [this, self] (const boost::system::error_code& ec, size_t transferred) {
			    handleReadRequest(ec, transferred);
			  });
}

void Session::write(std::string_view reply) {
  auto self(shared_from_this());
  boost::asio::async_write(_socket,
			   boost::asio::buffer(reply.data(), reply.size()),
			   [this, self](boost::system::error_code ec, size_t transferred) {
			     handleWriteReply(ec, transferred);
			   });
}

void Session::handleReadRequest(const boost::system::error_code& ec, size_t transferred) {
  boost::system::error_code ignore;
  _timer.cancel(ignore);
  if (!ec && _request.size() == transferred) {
    onReceiveRequest();
  }
  else {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' <<__func__ << ':'
	      << std::strerror(errno) << std::endl;
    boost::system::error_code ignore;
    _timer.cancel(ignore);
  }
}

void Session::handleWriteReply(const boost::system::error_code& ec, size_t transferred) {
  boost::system::error_code ignore;
  _timer.cancel(ignore);
  if (!ec) {
    if (!stopFlag)
      readHeader();
  }
  else {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' <<__func__ << ':'
	      << std::strerror(errno) << std::endl;
    boost::system::error_code ignore;
    _timer.cancel(ignore);
  }
}

void Session::asyncWait() {
  const static unsigned timeout = ProgramOptions::get("Timeout", 1);
  boost::system::error_code ignore;
  _timer.expires_from_now(std::chrono::seconds(timeout), ignore);
  auto self(shared_from_this());
  static const std::string function(__FUNCTION__);
  _timer.async_wait([self](const boost::system::error_code& err) {
		      if (err != boost::asio::error::operation_aborted) {
			std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":timeout" << std::endl;
			//boost::system::error_code ignore;
			//_socket.close(ignore);
			//_timer.expires_at(boost::asio::steady_timer::time_point::max(), ignore);
		      }
		    });
}

boost::asio::io_context TcpServer::_ioContext;
std::thread TcpServer::_thread;
std::list<TcpServer> TcpServer::_servers;

TcpServer::TcpServer(const std::string& port,
		     boost::asio::io_context& ioContext,
		     const boost::asio::ip::tcp::endpoint& endpoint)
  : _port(port), _acceptor(ioContext, endpoint) {
  accept();
}

void TcpServer::accept() {
  _acceptor.async_accept([this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
			   if (!ec)
			     std::make_shared<Session>(_port, std::move(socket))->start();
			   accept();
			 });
}

void TcpServer::run() {
  _ioContext.run();
}

bool TcpServer::startServers() {
  try {
    std::string tcpPortsStr = ProgramOptions::get("TcpPorts", std::string());
    std::vector<std::string> tcpPortsVector;
    utility::split(tcpPortsStr, tcpPortsVector, ",\n ");
    for (auto port : tcpPortsVector) {
      boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), std::atoi(port.c_str()));
      _servers.emplace_back(port, _ioContext, endpoint);
    }
    std::thread tmp(TcpServer::run);
    _thread.swap(tmp);
  }
  catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }
  return true;
}

void  TcpServer::joinThread() {
  _ioContext.stop();
  if (_thread.joinable()) {
    _thread.join();
    std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << " ... _thread joined ..." << std::endl;
  }
}
