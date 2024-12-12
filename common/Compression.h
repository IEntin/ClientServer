/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

namespace compression {

void compress(std::string& buffer, std::string& data);

void uncompress(std::string& buffer, std::string& data, size_t uncomprSize);

} // end of namespace compression
