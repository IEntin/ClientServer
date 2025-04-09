/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CompressionZSTD.h"

#include <cstring>
#include <stdexcept>
#include <zstd.h>

namespace compressionZSTD {

bool compress(std::string& buffer, std::string& data, int compressionLevel) {
  std::size_t dstSize = ZSTD_compressBound(data.size());
  buffer.resize(dstSize);
  std::size_t compressedSize = ZSTD_compress(buffer.data(), dstSize, data.data(), data.size(), compressionLevel);
  if (ZSTD_isError(compressedSize))
    throw std::runtime_error(ZSTD_getErrorName(compressedSize));
  data.resize(compressedSize);
  std::memcpy(data.data(), buffer.data(), compressedSize);
  return true;
}

bool uncompress(std::string& buffer, std::string& data) {
  std::size_t decompressedSize = ZSTD_getFrameContentSize(data.data(), data.size());
  if (ZSTD_isError(decompressedSize))
    throw std::runtime_error(ZSTD_getErrorName(decompressedSize));
  buffer.resize(decompressedSize);
  ZSTD_decompress(buffer.data(), buffer.size(), data.data(), data.size());
  data.resize(decompressedSize);
  std::memcpy(data.data(), buffer.data(), decompressedSize);
  return true;
}

} // end of namespace compressionZSTD
