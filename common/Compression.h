/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <cstdint>
#include <string_view>

enum class COMPRESSORS : uint8_t;

namespace compression {

std::string_view compress(std::string_view data);

std::string_view uncompress(std::string_view data, size_t uncomprSize);

} // end of namespace compression
