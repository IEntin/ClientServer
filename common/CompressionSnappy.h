/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <snappy.h>
#include <stdexcept>
#include <string>

#include <boost/static_string/static_string.hpp>

#include "Logger.h"

namespace compressionSnappy {

template <typename DATA>
bool compress(std::string& buffer, DATA& data) {
  if (snappy::Compress(&data.front(), data.size(), &buffer)) {
    data.assign(buffer.cbegin(), buffer.cend());
    return true;
  }
  LogError << "compress failed.\n";
  return false;
}

template <typename DATA>
bool uncompress(std::string& buffer, DATA& data) {
  if (!snappy::IsValidCompressedBuffer(&data.front(), data.size()))
    throw std::runtime_error("data is not valid.");      
  if (snappy::Uncompress(&data.front(), data.size(), &buffer)) {
    data.assign(buffer.cbegin(), buffer.cend());
    return true;
  }
  LogError << "uncompress failed.\n";
  return false;
}

} // end of namespace compressionSnappy
