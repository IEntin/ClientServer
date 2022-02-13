/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>
#include <vector>

struct FifoClientOptions;

namespace fifo {

class FifoClient {

  FifoClient() = delete;
  ~FifoClient() = delete;

  using Batch = std::vector<std::string>;

  static bool receive(int fd, std::ostream* dataStream);

  static bool processTask(const Batch& payload,
			  const FifoClientOptions& options,
			  int& fdWrite,
			  int& fdRead);

  static bool readBatch(int fd,
			size_t uncomprSize,
			size_t comprSize,
			bool bcompressed,
			std::ostream* dataStream);
 public:
  static bool run(const Batch& payload, const FifoClientOptions& options);
};

} // end of namespace fifo
