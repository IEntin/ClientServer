/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Client.h"
#include <string>
#include <vector>

namespace fifo {

class FifoClient : public Client {

  bool requestConnection() override;

  bool send(const std::string& fifoName, const std::vector<char>& msg) override;

  bool receive() override;

  bool readReply(size_t uncomprSize, size_t comprSize, bool bcompressed);

  const std::string _fifoName;
  const bool _setPipeSize;
  int _fdRead = -1;
  int _fdWrite = -1;

 public:

  FifoClient(const ClientOptions& options);

  ~FifoClient() override;

};

} // end of namespace fifo
