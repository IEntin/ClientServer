/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClient.h"

#include "Tcp.h"

namespace tcp {

TcpClient::TcpClient() : _socket(_ioContext) {
  if (!Tcp::setSocket(_socket))
    throw std::runtime_error(utility::createErrorString());
  auto lambda = [this] (
    const HEADER& header,
    std::string_view msgHash,
    const CryptoPP::SecByteBlock& pubKey,
    std::string_view signedAuth) {
    return Tcp::sendMsg(_socket, header, msgHash, pubKey, signedAuth);
  };
  if (!init(lambda))
    throw std::runtime_error("TcpClient::init failed");
  if (!receiveStatus())
    throw std::runtime_error("TcpClient::receiveStatus failed");
  Info << _socket.local_endpoint() << ' ' << _socket.remote_endpoint() << '\n';
}

void TcpClient::run() {
  start();
  Client::run();
}

bool TcpClient::send(Subtask& subtask) {
  try {
    boost::system::error_code ec;
    _socket.wait(boost::asio::ip::tcp::socket::wait_write, ec);
    if (ec) {
      Warn << ec.what() << '\n';
      return false;
    }
    Tcp::sendMsg(_socket, subtask._header, subtask._data);
    return true;
  }
  catch (const std::exception& e) {
    Warn << e.what() << '\n';
    return false;
  }
}

bool TcpClient::receive() {
  try {
    _response.erase(_response.cbegin(), _response.cend());
    boost::system::error_code ec;
    _socket.wait(boost::asio::ip::tcp::socket::wait_read, ec);
    if (ec) {
      Warn << ec.what() << '\n';
      return false;
    }
    HEADER header;
    if (!Tcp::readMsg(_socket, header, _response)) {
      return false;
    }
    _status = STATUS::NONE;
    return printReply();
  }
  catch (const std::exception& e) {
    Warn << e.what() << '\n';
    return false;
  }
}

bool TcpClient::receiveStatus() {
  if (_status != STATUS::NONE)
    return false;
  std::string clientIdStr;
  CryptoPP::SecByteBlock pubAreceived;
  if (!Tcp::readMsg(_socket, _header, clientIdStr, pubAreceived))
    return false;
  if (!DHFinish(clientIdStr, pubAreceived))
    return false;
  startHeartbeat();
  return true;
}

} // end of namespace tcp
