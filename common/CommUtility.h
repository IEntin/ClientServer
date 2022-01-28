#pragma once

#include <string>
#include <vector>

using Batch = std::vector<std::string>;

namespace commutility {

std::string_view buildReply(const Batch& batch);

size_t createPayload(const char* sourceName, Batch& payload);

} // end of namespace commutility
