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
  bool wakeupAcceptor();
  static bool destroy(const ClientOptions& options);
  int _fdReadS = -1;
  std::string _fifoName;
  static inline std::string _clientId;

 public:
  FifoClient(const ClientOptions& options);
  ~FifoClient() override;

  bool run() override;

  static void setCloseFlag(const ClientOptions& options);
};

} // end of namespace fifo
