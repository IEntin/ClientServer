/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/asio.hpp>

#include "Client.h"
#include "Tcp.h"

namespace tcp {

class TcpClient : public Client {

  bool send(const Subtask& subtask) override;
  bool receive() override;
  bool receiveStatus() override;

  template <typename EncryptorContainer>
  void sendSignature(EncryptorContainer& container) {
  auto lambda = [this] (
    const HEADER& header,
    std::string_view pubKeyAes,
    std::string_view signedAuth) -> bool {
    return Tcp::sendMessage(_socket, header, pubKeyAes, signedAuth);
  };
  constexpr std::size_t index = getEncryptorIndex();
  auto crypto = std::get<index>(container);
  if (!crypto->sendSignature(lambda))
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
