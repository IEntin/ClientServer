
/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CompressionLZ4.h"

#include <lz4.h>

#include <stdexcept>

namespace compressionLZ4 {

void compress(std::string& buffer, std::string& data) {
  std::size_t uncompressedSize = data.size();
  auto metadata = ioutility::toCharsBoost(uncompressedSize, true);
  std::size_t requiredCapacity = LZ4_compressBound(uncompressedSize) + ioutility::CONV_BUFFER_SIZE;
  buffer.resize(requiredCapacity);
  std::size_t compressedSize = LZ4_compress_default(data.data(),
						    buffer.data(),
						    data.size(),
						    buffer.capacity());
  if (compressedSize == 0)
    throw std::runtime_error("compress failed");
  buffer.assign(buffer.data(), compressedSize).append(metadata);
  data = buffer;
}

void uncompress(std::string& buffer, std::string& data) {
  std::string_view metadata(data.cend() - ioutility::CONV_BUFFER_SIZE, data.cend());
  std::size_t uncomprSize = 0;
  ioutility::fromChars(metadata, uncomprSize);
  data.resize(data.size() - metadata.size());
  buffer.resize(uncomprSize);
  ssize_t decomprSize = LZ4_decompress_safe(data.data(),
					    buffer.data(),
					    data.size(),
					    uncomprSize);
  if (decomprSize < 0)
    throw std::runtime_error("uncompress failed");
  data.assign(buffer.data(), decomprSize);
}

} // end of namespace compression
