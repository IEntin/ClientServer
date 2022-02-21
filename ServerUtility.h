#pragma once

#include "Compression.h"
#include <string>
#include <vector>

using Batch = std::vector<std::string>;

namespace serverutility {

std::string_view buildReply(const Batch& batch, const CompressionType& compression);

} // end of namespace commutility
