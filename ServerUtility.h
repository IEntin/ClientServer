/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>
#include <vector>

enum class COMPRESSORS : short unsigned int;

struct MemoryPool;

using Batch = std::vector<std::string>;

namespace serverutility {

std::string_view buildReply(const Batch& batch, COMPRESSORS compressor, MemoryPool& memoryPool);

} // end of namespace serverutility
