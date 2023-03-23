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
  static bool destroy(const ClientOptions& options);
  int _fdReadS = -1;
  int _fdWriteS = -1;
  int _fdReadA = -1;
  inline static int _fdWriteA = -1;
  static std::string _fifoName;

 public:
  FifoClient(const ClientOptions& options);
  ~FifoClient() override;

  bool run() override;

  static void setCloseFlag(const ClientOptions& options);
};

} // end of namespace fifo
