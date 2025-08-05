/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CompressionLZ4.h"

#include <cstring>
#include <lz4.h>
#include <stdexcept>

namespace compressionLZ4 {

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
  data.assign(buffer.cbegin(), buffer.cbegin() + compressedSize);
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
  std::size_t size = std::bit_cast<size_t>(decomprSize);
  data.assign(buffer.cbegin(), buffer.cbegin() + size);
}

} // end of namespace compression
