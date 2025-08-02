/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Tcp.h"

#include "Options.h"

namespace tcp {

thread_local std::string Tcp::_payload;

bool Tcp::setSocket(boost::asio::ip::tcp::socket& socket) {
  boost::system::error_code ec;
  static const auto ipAdress =
    boost::asio::ip::make_address(Options::_serverAddress, ec);
  if (ec) {
    switch (ec.value()) {
    case boost::asio::error::connection_refused:
      Info << ec.what() << '\n';
      return false;
    default:
      LogError << ec.what() << '\n';
      return false;
    }
  }
  static const boost::asio::ip::tcp::endpoint endpoint(ipAdress, Options::_tcpPort);
  socket.connect(endpoint, ec);
  if (ec) {
    switch (ec.value()) {
    case boost::asio::error::eof:
    case boost::asio::error::connection_refused:
    case boost::asio::error::connection_reset:
    case boost::asio::error::broken_pipe:
      Info << ec.what() << '\n';
      return false;
    default:
      Warn << ec.what() << '\n';
      return false;
    }
    return false;
  }
  socket.set_option(boost::asio::socket_base::reuse_address(true), ec);
  if (ec) {
    if (ec.value() == boost::asio::error::connection_reset)
      Info << ec.what() << '\n';
    return false;
  }
  return true;
}

void Tcp::shutdownSocket(boost::asio::ip::tcp::socket& socket) {
  boost::system::error_code ec;
  socket.shutdown(boost::asio::socket_base::shutdown_both, ec);
  if (!ec)
    socket.close(ec);
}

bool Tcp::readMessage(boost::asio::ip::tcp::socket& socket,
		      std::string& payload) {
  boost::system::error_code ec;
  std::size_t transferred [[maybe_unused]] = boost::asio::read_until(socket,
    boost::asio::dynamic_buffer(payload), ENDOFMESSAGE, ec);
  if (ec) {
    Info << ec.what() << '\n';
    return false;
  }
  if (payload.ends_with(ENDOFMESSAGE)) {
    payload.erase(payload.size() - ENDOFMESSAGESZ);     
    return true;
  }
  else {
    LogError << "ENDOFMESSAGE not found\n";
    return false;
  }
}

bool Tcp::readMessage(boost::asio::ip::tcp::socket& socket,
		      HEADER& header,
		      std::string& payload) {
  _payload.clear();
  if (!readMessage(socket, _payload))
    return false;
  if (!deserialize(header, _payload.data()))
    return false;
  std::size_t payloadSize = _payload.size() - HEADER_SIZE;
  if (payloadSize > 0) {
    payload.resize(payloadSize);
    std::copy(_payload.cbegin() + HEADER_SIZE, _payload.cbegin() + HEADER_SIZE + payloadSize, payload.begin());
    return true;
  }
  return false;
}

bool Tcp::readMessage(boost::asio::ip::tcp::socket& socket,
		      HEADER& header,
		      std::string& payload1,
		      std::string& payload2) {
  _payload.clear();
  if (!readMessage(socket, _payload))
    return false;
  deserialize(header, _payload.data());
  assert(!isCompressed(header) && "not compressed");
  std::size_t payload1Sz = extractUncompressedSize(header);
  std::size_t payload2Sz = extractParameter(header);
  payload1.resize(payload1Sz);
  payload2.resize(payload2Sz);
  unsigned shift = HEADER_SIZE;
  if (payload1Sz > 0)
    std::copy(_payload.cbegin() + shift, _payload.cbegin() + shift + payload1Sz, payload1.begin());
  shift += payload1Sz;
  if (payload2Sz > 0)
    std::copy(_payload.cbegin() + shift, _payload.cbegin() + shift + payload2Sz, payload2.begin());
  return true;
}

bool Tcp::readMessage(boost::asio::ip::tcp::socket& socket,
		      HEADER& header,
		      std::string& payload1,
		      std::string& payload2,
		      std::string& payload3) {
  _payload.clear();
  if (!readMessage(socket, _payload))
    return false;
  deserialize(header, _payload.data());
  assert(!isCompressed(header) && "not compressed");
  std::size_t payload1Sz = extractReservedSz(header);
  std::size_t payload2Sz = extractUncompressedSize(header);
  std::size_t payload3Sz = extractParameter(header);
  payload1.resize(payload1Sz);
  payload2.resize(payload2Sz);
  payload3.resize(payload3Sz);
  unsigned shift = HEADER_SIZE;
  if (payload1Sz > 0)
    std::copy(_payload.cbegin() + shift, _payload.cbegin() + shift + payload1Sz, payload1.begin());
  shift += payload1Sz;
  if (payload2Sz > 0)
    std::copy(_payload.cbegin() + shift, _payload.cbegin() + shift + payload2Sz, payload2.begin());
  shift += payload2Sz;
  if (payload3Sz > 0)
    std::copy(_payload.cbegin() + shift, _payload.cbegin() + shift + payload3Sz, payload3.begin());
  return true;
}

} // end of namespace tcp
