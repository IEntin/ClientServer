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
    Fifo::sendMessage(false, Options::_acceptorName, _authenticationHeader, _primarySignatureWithKey, _primaryPubKeyAes,
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
    return sentSignature;
  }

  std::string _fifoName;

 public:
  FifoClient();
  ~FifoClient() override;

  void run() override;
};

} // end of namespace fifo
