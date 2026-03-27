/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Client.h"
#include "Fifo.h"

using namespace encryptortemplates;

namespace fifo {

class FifoClient : public Client {

  bool send(const Subtask& subtask) override;
  bool receive() override;
  bool receiveStatus() override;

  template <typename EncryptorContainer>
  bool wakeupAcceptor(EncryptorContainer& container) {
    auto lambda = [] (const HEADER& header,
		      std::string_view pubKeyAesServer,
		      std::string_view signedAuth) -> bool {
      return Fifo::sendMessage(false, Options::_acceptorName, header, pubKeyAesServer, signedAuth);
    };
    bool sentSignature = false;
    if (CryptoSodiumPtr* ptr = std::get_if<CryptoSodiumPtr>(&container))
      sentSignature = (*ptr)->sendSignature(lambda);
    else if (CryptoPlPlPtr* ptr = std::get_if<CryptoPlPlPtr>(&container))
      sentSignature = (*ptr)->sendSignature(lambda);
    return sentSignature;
  }

  std::string _fifoName;

 public:
  FifoClient();
  ~FifoClient() override;

  void run() override;
};

} // end of namespace fifo
