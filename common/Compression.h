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

template <typename C>
std::string_view compress(std::string_view uncompressed, C& compressed) {
  compressed.reserve(LZ4_compressBound(uncompressed.size()));
  size_t compressedSize = LZ4_compress_default(uncompressed.data(),
					       compressed.data(),
					       uncompressed.size(),
					       compressed.capacity());
  if (compressedSize == 0)
    throw std::runtime_error("compress failed");
  return { compressed.data(), compressedSize };
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
