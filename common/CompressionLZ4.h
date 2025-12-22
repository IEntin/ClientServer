/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

#include <boost/static_string/static_string.hpp>

#include "IOUtility.h"

namespace compressionLZ4 {

void compress(std::string& buffer, std::string& data);

void uncompress(std::string& buffer, std::string& data);

} // end of namespace compression
