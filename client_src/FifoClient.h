/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Client.h"

namespace fifo {

class FifoClient : public Client {

  bool send(Subtask& subtask) override;
  bool receive() override;
  bool receiveStatus() override;
  void close() override;
  bool wakeupAcceptor();
  std::string_view _acceptorName;
  std::string _fifoName;
  std::string _response;

 public:
  FifoClient();
  ~FifoClient() override;

  void run() override;
};

} // end of namespace fifo
