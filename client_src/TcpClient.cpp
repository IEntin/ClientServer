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
    std::string_view signedAuth) -> bool {
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
    if (!Tcp::readMessage(_socket, header, _response))
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
  std::string encodedPeerPubKeyAes;
  std::string type;
  auto lambda = [this] (boost::asio::ip::tcp::socket& socket,
			HEADER& header,
			std::string& clientIdStr,
			std::string& encodedPeerPubKeyAes,
			std::string& type) -> bool {
    if (!Tcp::readMessage(socket, header, clientIdStr, encodedPeerPubKeyAes))
      throw std::runtime_error("readMessage failed");
    type = "tcp";
    ioutility::fromChars(clientIdStr, _clientId);
    return true;
  };
  lambda(_socket, _header, clientIdStr, encodedPeerPubKeyAes, type);
  return processStatus(encodedPeerPubKeyAes, type);
}

} // end of namespace tcp
