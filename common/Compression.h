/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string_view>

namespace compression {

void compress(std::string& data);

void uncompress(std::string& data, size_t uncomprSize);

} // end of namespace compression
