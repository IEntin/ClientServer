/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClient.h"

#include "Tcp.h"

namespace tcp {

TcpClient::TcpClient() : _socket(_ioContext) {
  if (!Tcp::setSocket(_socket))
    throw std::runtime_error(ioutility::createErrorString());
  auto lambda = [this]<typename T1, typename T2, typename T3> (
    const HEADER& header,
    const T1& msgHash,
    const T2& pubKeyAes,
    const T3& signedAuth) {
    return Tcp::sendMessage(_socket, header, msgHash, pubKeyAes, signedAuth);
  };
  if (!_crypto->sendSignature(lambda, _status))
    throw std::runtime_error("TcpClient::init failed");
  if (!receiveStatus())
    throw std::runtime_error("TcpClient::receiveStatus failed");
  Info << _socket.local_endpoint() << ' ' << _socket.remote_endpoint() << '\n';
}

void TcpClient::run() {
  start();
  Client::run();
}

bool TcpClient::send(const Subtask& subtask) {
  try {
    boost::system::error_code ec;
    _socket.wait(boost::asio::ip::tcp::socket::wait_write, ec);
    if (ec) {
      Warn << ec.what() << '\n';
      return false;
    }
    Tcp::sendMessage(_socket, subtask._header, subtask._data);
    return true;
  }
  catch (const std::exception& e) {
    Warn << e.what() << '\n';
    return false;
  }
}

bool TcpClient::receive() {
  try {
    boost::system::error_code ec;
    _socket.wait(boost::asio::ip::tcp::socket::wait_read, ec);
    if (ec) {
      Warn << ec.what() << '\n';
      return false;
    }
    HEADER header;
    if (!Tcp::readMessage(_socket, header, _response)) {
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
  std::vector<unsigned char> pubKeyAesServer;
  if (!Tcp::readMessage(_socket, _header, clientIdStr, pubKeyAesServer))
    return false;
  if (!DHFinish(clientIdStr, pubKeyAesServer))
    return false;
  startHeartbeat();
  return true;
}

} // end of namespace tcp
