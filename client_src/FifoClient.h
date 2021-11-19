#pragma once

#include <string>
#include <vector>

namespace fifo {

using Batch = std::vector<std::string>;

bool receive(int fd, std::ostream* dataStream);

bool readBatch(int fd,
	       size_t uncomprSize,
	       size_t comprSize,
	       bool bcompressed,
	       std::ostream* dataStream);

bool run(const Batch& payload,
	 bool runLoop,
	 unsigned maxNumberIterations,
	 std::ostream* dataStream,
	 std::ostream* instrStream);

bool singleIteration(const Batch& payload,
		     int fdWrite,
		     int fdRead,
		     std::ostream* pstream);

bool preparePackage(const Batch& payload, Batch& modified);

} // end of namespace fifo
