/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Client.h"
#include <string>
#include <vector>

struct FifoClientOptions;

namespace fifo {

class FifoClient : public Client {

  using Batch = std::vector<std::string>;

  bool receive();

  bool processTask(const Batch& payload);

  bool readBatch(size_t uncomprSize, size_t comprSize, bool bcompressed);

  const FifoClientOptions& _options;
  const std::string _fifoName;
  int _fdRead = -1;
  int _fdWrite = -1;
 public:

  FifoClient(const FifoClientOptions& _options);

  ~FifoClient() = default;

  bool run(const Batch& payload);
};

} // end of namespace fifo
