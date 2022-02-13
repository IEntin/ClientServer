#pragma once

#include <string>
#include <vector>

using Batch = std::vector<std::string>;

enum class COMPRESSORS : unsigned short;

namespace serverutility {

std::string_view buildReply(const Batch& batch, const std::pair<COMPRESSORS, bool>& compression);

} // end of namespace commutility
