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
  void createSignalWatcher();
  std::string _clientId;
  std::string _fifoName;
  RunnableWeakPtr _signalWatcher;

 public:
  FifoClient(const ClientOptions& options);
  ~FifoClient() override;

  bool run() override;
};

} // end of namespace fifo
