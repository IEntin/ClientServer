/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Client.h"
#include <string>
#include <vector>

namespace fifo {

class FifoClient : public Client {

  bool send(const Subtask& subtask) override;
  bool receive() override;
  bool receiveStatus() override;
  bool destroySession() override;
  bool readReply(const HEADER& header);
  bool wakeupAcceptor();

  std::string _fifoName;
  int _fdRead = -1;
  int _fdWrite = -1;

 public:
  FifoClient(const ClientOptions& options);
  ~FifoClient() override;

  bool run() override;

  static void setStopFlag(const ClientOptions& options);
};

} // end of namespace fifo
