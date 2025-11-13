/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClient.h"

#include "CryptoCommon.h"

namespace tcp {

TcpClient::TcpClient() : _socket(_ioContext) {
  if (!Tcp::setSocket(_socket))
    throw std::runtime_error(ioutility::createErrorString());
#ifdef CRYPTOVARIANT
  sendSignature(_encryptorVariant);
#elifdef CRYPTOTUPLE
   sendSignature(_encryptorTuple);
#endif
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
    std::array<std::reference_wrapper<std::string>, 1> array{ std::ref(_response) };
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
  std::string encodedPeerPubKeyAes;
  std::string type;
  auto lambda = [this] (boost::asio::ip::tcp::socket& socket,
			HEADER& header,
			std::string& clientIdStr,
			std::string& encodedPeerPubKeyAes,
			std::string& type) -> bool {
    std::array<std::reference_wrapper<std::string>, 2> array{ std::ref(clientIdStr),
							      std::ref(encodedPeerPubKeyAes) };
    if (!Tcp::readMessage(socket, header, array))
      throw std::runtime_error("readMessage failed");
    type = "tcp";
    ioutility::fromChars(clientIdStr, _clientId);
    return true;
  };
  lambda(_socket, _header, clientIdStr, encodedPeerPubKeyAes, type);
  return processStatus(encodedPeerPubKeyAes, type);
}

} // end of namespace tcp
