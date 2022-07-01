/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>
#include <vector>

enum class COMPRESSORS : int;

using Response = std::vector<std::string>;

namespace serverutility {

std::string_view buildReply(const Response& response, COMPRESSORS compressor);

} // end of namespace serverutility
