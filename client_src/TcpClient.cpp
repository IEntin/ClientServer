/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClient.h"

namespace tcp {

TcpClient::TcpClient() : _socket(_ioContext) {
  if (!Tcp::setSocket(_socket))
    throw std::runtime_error(ioutility::createErrorString());
  sendSignature();
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

void TcpClient::sendSignature() {
  bool sentSignature =
  Tcp::sendMessage(_socket, _authenticationHeader,
		   _primarySignatureWithKey, _primaryPubKeyAes,
		   _secondarySignatureWithKey, _secondaryPubKeyAes);
  if (sentSignature)
    sentSignature = sendSignatureCommon();
  if (!sentSignature)
    throw std::runtime_error("TcpClient::init failed");
  if (!receiveStatus())
    throw std::runtime_error("TcpClient::receiveStatus failed");
  Info << _socket.local_endpoint() << ' ' << _socket.remote_endpoint() << '\n';
}

bool TcpClient::receive() {
  try {
    boost::system::error_code ec;
    _socket.wait(boost::asio::ip::tcp::socket::wait_read, ec);
    if (ec) {
      if (ec == boost::asio::error::interrupted)
	Info << ec.what() << '\n';
      else
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
  std::string primaryPeerPubKeyAes;
  std::string secondaryPeerPubKeyAes;
  std::string type = "tcp";
  auto lambda = [this] (boost::asio::ip::tcp::socket& socket,
			HEADER& header,
			std::string& clientIdStr,
			std::string& primaryPeerPubKeyAes,
			std::string& secondaryPeerPubKeyAes) -> bool {
    std::array<std::reference_wrapper<std::string>, 3> array{ std::ref(clientIdStr),
                                                              std::ref(primaryPeerPubKeyAes),
							      std::ref(secondaryPeerPubKeyAes) };
    if (!Tcp::readMessage(socket, header, array))
      throw std::runtime_error("readMessage failed");
    ioutility::fromChars(clientIdStr, _clientId);
    return true;
  };
  lambda(_socket, _header, clientIdStr, primaryPeerPubKeyAes, secondaryPeerPubKeyAes);
  return processStatus(primaryPeerPubKeyAes, type, secondaryPeerPubKeyAes);
}

} // end of namespace tcp
