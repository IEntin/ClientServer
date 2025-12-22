/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CompressionSnappy.h"

#include <stdexcept>

#include <snappy.h>

#include "Logger.h"

namespace compressionSnappy {

bool compress(std::string& buffer, std::string& data) {
  if (snappy::Compress(data.data(), data.size(), &buffer)) {
    data.assign(buffer.cbegin(), buffer.cend());
    return true;
  }
  LogError << "compress failed.\n";
  return false;
}

bool uncompress(std::string& buffer, std::string& data) {
  if (!snappy::IsValidCompressedBuffer(data.data(), data.size()))
    throw std::runtime_error("data is not valid.");      
  if (snappy::Uncompress(data.data(), data.size(), &buffer)) {
    data.swap(buffer);
    return true;
  }
  LogError << "uncompress failed.\n";
  return false;
}

} // end of namespace compressionSnappy
