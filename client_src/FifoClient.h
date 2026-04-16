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
  bool sendSignature(EncryptorContainer& container) {
    auto lambda = [] (const HEADER& header,
		      std::string_view signedAuth,
		      std::string_view pubKeyAesServer
		      ) -> bool {
      return Fifo::sendMessage(false, Options::_acceptorName, header, signedAuth, pubKeyAesServer );
    };
    bool sentSignature = false;
    if (CryptoSodiumPtr* ptr = std::get_if<CryptoSodiumPtr>(&container)) {
      CryptoWeakSodiumPtr weak = *ptr;
      if (auto encryptor = weak.lock())
	sentSignature = encryptor->sendSignature(lambda);
    }
    if (CryptoPlPlPtr* ptr = std::get_if<CryptoPlPlPtr>(&container)) {
      CryptoWeakPlPlPtr weak = *ptr;
      if (auto encryptor = weak.lock())
	sentSignature = encryptor->sendSignature(lambda);
    }
    return sentSignature;
  }

  std::string _fifoName;

 public:
  FifoClient();
  ~FifoClient() override;

  void run() override;
};

} // end of namespace fifo
