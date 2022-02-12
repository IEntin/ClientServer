/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>
#include <vector>

struct FifoClientOptions;

namespace fifo {

using Batch = std::vector<std::string>;

bool receive(int fd, std::ostream* dataStream);

bool run(const Batch& payload, const FifoClientOptions& options);

bool processTask(const Batch& payload,
		 const FifoClientOptions& options,
		 int& fdWrite,
		 int& fdRead);

} // end of namespace fifo
