#pragma once

#include "Compression.h"
#include <string>
#include <vector>

using Batch = std::vector<std::string>;

namespace serverutility {

std::string_view buildReply(const Batch& batch, COMPRESSORS compressor, MemoryPool& memoryPool);

} // end of namespace serverutility
