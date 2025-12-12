/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>
#include <lz4.h>
#include <stdexcept>

#include <boost/static_string/static_string.hpp>

#include "IOUtility.h"

namespace compressionLZ4 {

template <typename DATA>
void compress(std::string& buffer, DATA& data) {
  std::size_t uncompressedSize = data.size();
  auto metadata = ioutility::toCharsBoost(uncompressedSize, true);
  std::size_t requiredCapacity = LZ4_compressBound(uncompressedSize);
  if (requiredCapacity > buffer.capacity())
    buffer.resize(requiredCapacity);
  std::size_t compressedSize = LZ4_compress_default(&*data.begin(),
						    &*buffer.begin(),
						    data.size(),
						    buffer.capacity());
  if (compressedSize == 0)
    throw std::runtime_error("compress failed");
  data.assign(buffer.cbegin(), buffer.cbegin() + compressedSize);
  data.insert(data.cend(), metadata.cbegin(), metadata.cend());
}

template <typename DATA>
void uncompress(std::string& buffer, DATA& data) {
  DATA metadata;
  metadata.assign(data.cend() - ioutility::CONV_BUFFER_SIZE, data.cend());
  std::size_t uncomprSize = 0;
  ioutility::fromChars(metadata, uncomprSize);
  data.resize(data.size() - metadata.size());
  if (uncomprSize > buffer.capacity())
    buffer.resize(uncomprSize);
  ssize_t decomprSize = LZ4_decompress_safe(&*data.begin(),
					    &*buffer.begin(),
					    data.size(),
					    uncomprSize);
  if (decomprSize < 0)
    throw std::runtime_error("uncompress failed");
  std::size_t size = std::bit_cast<size_t>(decomprSize);
  data.assign(buffer.cbegin(), buffer.cbegin() + size);
}

} // end of namespace compression
