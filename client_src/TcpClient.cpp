/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClient.h"

#include "CryptoDefinitions.h"
#include "Tcp.h"

namespace tcp {

TcpClient::TcpClient() : _socket(_ioContext) {
  if (!Tcp::setSocket(_socket))
    throw std::runtime_error(ioutility::createErrorString());
  auto lambda = [this] (
    const HEADER& header,
    std::string_view msgHash,
    std::string_view pubKeyAes,
    std::string_view signedAuth) {
    return Tcp::sendMessage(_socket, header, msgHash, pubKeyAes, signedAuth);
  };
  constexpr unsigned long index = cryptodefinitions::getEncryptionIndex();
  auto crypto = std::get<index>(_crypto);
  if (!crypto->sendSignature(lambda))
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
    std::array<std::reference_wrapper<std::string>, 1> array = { std::ref(_response) };
    if (!Tcp::readMessage(_socket, header, array))
      return false;
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
  std::string encodedPubKeyAesServer;
  std::string dummy;
  std::array<std::reference_wrapper<std::string>, 3> array = { dummy, std::ref(clientIdStr), std::ref(encodedPubKeyAesServer) };
  if (!Tcp::readMessage(_socket, _header, array))
    return false;
  ioutility::fromChars(clientIdStr, _clientId);
  try {
    cryptodefinitions::clientKeyExchange(_crypto, encodedPubKeyAesServer);
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
    return false;
  }
  if (!displayOverload("tcp"))
    return false;
  startHeartbeat();
  return true;
}

} // end of namespace tcp
