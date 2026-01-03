/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Client.h"
#include "Fifo.h"

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
    auto crypto = std::get<getEncryptorIndex()>(container);
    return crypto->sendSignature(lambda);
  }

  std::string _fifoName;

 public:
  FifoClient();
  ~FifoClient() override;

  void run() override;
};

} // end of namespace fifo
