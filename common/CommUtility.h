#pragma once

#include <string>
#include <vector>

using Batch = std::vector<std::string>;

namespace commutility {

std::string_view buildReply(const Batch& batch);

size_t createPayload(const char* sourceName, Batch& payload);

std::string readFileContent(const std::string& name);

} // end of namespace commutility
