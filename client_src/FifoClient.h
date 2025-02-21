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
  std::string _fifoName;

 public:
  FifoClient();
  ~FifoClient() override;

  void run() override;
};

} // end of namespace fifo
