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

inline size_t compressBound(size_t uncompressedSize) {
  return LZ4_compressBound(uncompressedSize);
}

template <typename C>
std::string_view compress(C& buffer, size_t uncompressedSize) {
  size_t compressedSize = LZ4_compress_default(buffer.data(),
					       buffer.data() + uncompressedSize,
					       uncompressedSize,
					       buffer.capacity());
  if (compressedSize == 0)
    throw std::runtime_error("compress failed");
  return { buffer.data() + uncompressedSize, compressedSize };
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
