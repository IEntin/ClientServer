/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/asio.hpp>

#include "Client.h"
#include "Tcp.h"

using namespace encryptortemplates;

namespace tcp {

class TcpClient : public Client {

  bool send(const Subtask& subtask) override;
  bool receive() override;
  bool receiveStatus() override;

  template <typename EncryptorContainer>
  void sendSignature(EncryptorContainer& container) {
    Tcp::sendMessage(_socket, _authenticationHeader, _primarySignatureWithKey, _primaryPubKeyAes,
		     _secondarySignatureWithKey, _secondaryPubKeyAes);
  bool sentSignature = false;
  if (CryptoSodiumPtr* ptr = std::get_if<CryptoSodiumPtr>(&container)) {
    CryptoWeakSodiumPtr weak = *ptr;
    if (auto encryptor = weak.lock())
      sentSignature = encryptor->sendSignature();
  }
  if (CryptoPlPlPtr* ptr = std::get_if<CryptoPlPlPtr>(&container)) {
    CryptoWeakPlPlPtr weak = *ptr;
    if (auto encryptor = weak.lock())
      sentSignature = encryptor->sendSignature();
  }
  if (!sentSignature)
    throw std::runtime_error("TcpClient::init failed");
  if (!receiveStatus())
    throw std::runtime_error("TcpClient::receiveStatus failed");
  Info << _socket.local_endpoint() << ' ' << _socket.remote_endpoint() << '\n';
  }

  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::socket _socket;
 public:
  TcpClient();
  ~TcpClient() override = default;
  void run() override;
};

} // end of namespace tcp
