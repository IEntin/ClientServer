#pragma once

#include <string>
#include <vector>

using Batch = std::vector<std::string>;

enum class COMPRESSORS : unsigned short;

namespace commutility {

std::string_view buildReply(const Batch& batch, const std::pair<COMPRESSORS, bool>& compression);

size_t createPayload(const char* sourceName, Batch& payload);

std::string readFileContent(const std::string& name);

} // end of namespace commutility
