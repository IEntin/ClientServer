/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CompressionSnappy.h"

#include <snappy.h>
#include <stdexcept>

#include "Logger.h"

namespace compressionSnappy {

bool compress(std::string& buffer, std::string& data) {
  if (snappy::Compress(data.data(), data.size(), &buffer)) {
    std::swap(buffer, data);
    return true;
  }
  LogError << "compress failed.\n";
  return false;
}

bool uncompress(std::string& buffer, std::string& data) {
  if (!snappy::IsValidCompressedBuffer(data.data(), data.size()))
    throw std::runtime_error("data is not valid.");      
  if (snappy::Uncompress(data.data(), data.size(), &buffer)) {
    std::swap(buffer, data);
    return true;
  }
  LogError << "uncompress failed.\n";
  return false;
}

} // end of namespace compressionSnappy
