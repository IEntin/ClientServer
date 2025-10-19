/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CompressionLZ4.h"

#include <cstring>
#include <lz4.h>
#include <stdexcept>

#include "IOUtility.h"

namespace compressionLZ4 {

void compress(std::string& buffer, std::string& data) {
  std::size_t uncompressedSize = data.size();
  std::string metadata;
  ioutility::toChars(uncompressedSize, metadata);
  metadata.resize(ioutility::CONV_BUFFER_SIZE);
  std::size_t requiredCapacity = LZ4_compressBound(uncompressedSize);
  if (requiredCapacity > buffer.capacity())
    buffer.reserve(requiredCapacity);
  std::size_t compressedSize = LZ4_compress_default(data.data(),
						    buffer.data(),
						    data.size(),
						    buffer.capacity());
  if (compressedSize == 0)
    throw std::runtime_error("compress failed");
  data.assign(buffer.cbegin(), buffer.cbegin() + compressedSize);
  data.append(metadata);
}

void uncompress(std::string& buffer, std::string& data) {
  auto metadata = data.substr(data.size() - ioutility::CONV_BUFFER_SIZE);
  std::size_t uncomprSize = 0;
  ioutility::fromChars(metadata, uncomprSize);
  data.erase(data.size() - ioutility::CONV_BUFFER_SIZE);
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
