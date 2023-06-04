/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Client.h"

namespace fifo {

class FifoClient : public Client {

  bool send(const Subtask& subtask) override;
  bool receive() override;
  bool receiveStatus() override;
  bool wakeupAcceptor();
  static inline std::string _fifoName;
  static inline std::string _clientId;

 public:
  FifoClient(const ClientOptions& options);
  ~FifoClient() override;

  bool run() override;

  static void onClose();
};

} // end of namespace fifo
