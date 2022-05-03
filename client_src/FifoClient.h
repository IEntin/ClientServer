/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Client.h"
#include <string>
#include <vector>

struct FifoClientOptions;

namespace fifo {

class FifoClient : protected Client {

  bool processTask() override;

  bool receive();

  bool readBatch(size_t uncomprSize, size_t comprSize, bool bcompressed);

  const std::string _fifoName;
  int _fdRead = -1;
  int _fdWrite = -1;
 public:

  FifoClient(const FifoClientOptions& options);

  ~FifoClient();

  bool run();
};

} // end of namespace fifo
