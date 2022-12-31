/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Client.h"
#include <string>
#include <vector>

namespace fifo {

class FifoClient : public Client {

  bool send(const std::vector<char>& msg) override;

  bool receive() override;

  bool receiveStatus() override;

  bool readReply(const HEADER& header);

  bool wakeupAcceptor();

  std::string _fifoName;
  int _fdRead = -1;
  int _fdWrite = -1;

 public:

  FifoClient(const ClientOptions& options);

  ~FifoClient() override;

  bool run() override;

};

} // end of namespace fifo
