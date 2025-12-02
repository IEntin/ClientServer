/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <cstring>
#include <stdexcept>
#include <string>
#include <zstd.h>

#include <boost/static_string/static_string.hpp>

namespace compressionZSTD {

template <typename DATA>
bool compress(std::string& buffer, DATA& data, int compressionLevel = -3) {
  std::size_t dstSize = ZSTD_compressBound(data.size());
  buffer.resize(dstSize);
  std::size_t compressedSize = ZSTD_compress(&buffer.front(), dstSize, &data.front(), data.size(), compressionLevel);
  if (ZSTD_isError(compressedSize))
    throw std::runtime_error(ZSTD_getErrorName(compressedSize));
  data.assign(buffer.cbegin(), buffer.cbegin() + compressedSize);
  return true;
}

template <typename DATA>
bool uncompress(std::string& buffer, DATA& data) {
  std::size_t decompressedSize = ZSTD_getFrameContentSize(&data.front(), data.size());
  if (ZSTD_isError(decompressedSize))
    throw std::runtime_error(ZSTD_getErrorName(decompressedSize));
  buffer.resize(decompressedSize);
  ZSTD_decompress(&buffer.front(), decompressedSize, &data.front(), data.size());
  data.assign(buffer.cbegin(), buffer.cend());
  return true;
}

} // end of namespace compressionZSTD
