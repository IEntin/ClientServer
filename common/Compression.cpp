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
  std::size_t requiredCapacity = LZ4_compressBound(data.size());
  if (requiredCapacity > buffer.capacity())
    buffer.reserve(requiredCapacity);
  std::size_t compressedSize = LZ4_compress_default(data.data(),
						    buffer.data(),
						    data.size(),
						    buffer.capacity());
  if (compressedSize == 0)
    throw std::runtime_error("compress failed");
  data.resize(compressedSize);
  std::memcpy(data.data(), buffer.data(), compressedSize);
}

void uncompress(std::string& buffer, std::string& data, std::size_t uncomprSize) {
  if (uncomprSize > buffer.capacity())
    buffer.reserve(uncomprSize);
  ssize_t decomprSize = LZ4_decompress_safe(data.data(),
					    buffer.data(),
					    data.size(),
					    uncomprSize);
  if (decomprSize < 0)
    throw std::runtime_error("uncompress failed");
  std::size_t size = static_cast<size_t>(decomprSize);
  //LogAlways << "size=" << size << " data.capacity()=" << data.capacity() << '\n';
  data.resize(size);
  std::memcpy(data.data(), buffer.data(), size);
}

} // end of namespace compression
