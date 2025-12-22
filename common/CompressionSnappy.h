/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

namespace compressionSnappy {

bool compress(std::string& buffer, std::string& data);

bool uncompress(std::string& buffer, std::string& data);

} // end of namespace compressionSnappy
