/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>
#include <vector>

enum class COMPRESSORS : int;

struct MemoryPool;

using Response = std::vector<std::string>;

namespace serverutility {

std::string_view buildReply(const Response& response, COMPRESSORS compressor, MemoryPool& memoryPool);

} // end of namespace serverutility
