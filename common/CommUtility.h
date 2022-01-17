#pragma once

#include <string_view>
#include <vector>

using Batch = std::vector<std::string>;

namespace commutility {

std::string_view buildReply(const Batch& batch);

 } // end of namespace commutility
