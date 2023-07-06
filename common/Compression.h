/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include "lz4.h"
#include <stdexcept>
#include <string_view>

namespace compression {

inline COMPRESSORS translateName(std::string_view compressorStr) {
  COMPRESSORS compressor = compressorStr == "LZ4" ? COMPRESSORS::LZ4 : COMPRESSORS::NONE;
  return compressor;
}

template <typename B>
void compress(B& data) {
  static thread_local B buffer;
  buffer.clear();
  buffer.resize(LZ4_compressBound(data.size()));
  size_t compressedSize = LZ4_compress_default(data.data(),
					       buffer.data(),
					       data.size(),
					       buffer.capacity());
  if (compressedSize == 0)
    throw std::runtime_error("compress failed");
  buffer.resize(compressedSize);
  data.swap(buffer);
}

template <typename B>
void uncompress(B& data, size_t uncomprSize) {
  static thread_local B uncompressed;
  uncompressed.clear();
  uncompressed.resize(uncomprSize);
  ssize_t decomprSize = LZ4_decompress_safe(data.data(),
					    uncompressed.data(),
					    data.size(),
					    uncomprSize);
  if (decomprSize != static_cast<ssize_t>(uncomprSize))
    throw std::runtime_error("uncompress failed");
  data.swap(uncompressed);
}

} // end of namespace compression
