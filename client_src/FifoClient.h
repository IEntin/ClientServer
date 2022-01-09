/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>
#include <vector>

namespace fifo {

using Batch = std::vector<std::string>;

bool receive(int fd, std::ostream* dataStream);

bool run(const Batch& payload,
	 bool runLoop,
	 unsigned maxNumberTasks,
	 std::ostream* dataStream,
	 std::ostream* instrStream);

bool processTask(const Batch& payload, int& fdWrite, int& fdRead, std::ostream* pstream);

void stop();

} // end of namespace fifo
