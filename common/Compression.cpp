/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Compression.h"

#include <lz4.h>
#include <stdexcept>
#include <vector>

#include "Logger.h"

namespace compression {

void compress(std::string& buffer, std::string& data) {
  buffer.erase(buffer.begin(), buffer.end());
  //LogAlways << "\t### " << buffer.capacity() << '\n';
  std::size_t requiredCapacity = LZ4_compressBound(data.size());
  if (requiredCapacity > buffer.capacity())
    buffer.reserve(requiredCapacity);
  std::size_t compressedSize = LZ4_compress_default(data.data(),
						    buffer.data(),
						    data.size(),
						    buffer.capacity());
  if (compressedSize == 0)
    throw std::runtime_error("compress failed");
  data.assign(buffer.data(), compressedSize);
}

void uncompress(std::string& buffer, std::string& data, std::size_t uncomprSize) {
  buffer.erase(buffer.begin(), buffer.end());
  //LogAlways << "\t### " << buffer.capacity() << '\n';
  if (uncomprSize > buffer.capacity())
    buffer.reserve(uncomprSize);
  ssize_t decomprSize = LZ4_decompress_safe(data.data(),
					    buffer.data(),
					    data.size(),
					    uncomprSize);
  if (decomprSize < 0)
    throw std::runtime_error("uncompress failed");
  data.assign(buffer.data(), static_cast<size_t>(decomprSize));
}

} // end of namespace compression
