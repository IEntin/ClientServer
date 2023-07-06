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
  static thread_local std::string buffer;
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

template <typename U>
void uncompress(std::string_view compressed, U& uncompressed) {
  ssize_t decomprSize = LZ4_decompress_safe(compressed.data(),
					    uncompressed.data(),
					    compressed.size(),
					    uncompressed.size());
  if (decomprSize != static_cast<ssize_t>(uncompressed.size()))
    throw std::runtime_error("uncompress failed");
}

} // end of namespace compression
