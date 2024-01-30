/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Compression.h"

#include <lz4.h>
#include <stdexcept>
#include <vector>

#include "Header.h"
#include "Logger.h"

namespace compression {

COMPRESSORS translateName(std::string_view compressorStr) {
  COMPRESSORS compressor = compressorStr == "LZ4" ? COMPRESSORS::LZ4 : COMPRESSORS::NONE;
  return compressor;
}

std::string_view compress(std::string_view data) {
  static thread_local std::vector<char> buffer;
  //LogAlways << "\t### " << buffer.capacity() << '\n';
  buffer.reserve(LZ4_compressBound(data.size()));
  std::size_t compressedSize = LZ4_compress_default(data.data(),
						    buffer.data(),
						    data.size(),
						    buffer.capacity());
  if (compressedSize == 0)
    throw std::runtime_error("compress failed");
  return { buffer.data(), compressedSize };
}

std::string_view uncompress(std::string_view data, std::size_t uncomprSize) {
  static thread_local std::vector<char> uncompressed;
  //LogAlways << "\t### " << uncompressed.capacity() << '\n';
  uncompressed.reserve(uncomprSize);
  ssize_t decomprSize = LZ4_decompress_safe(data.data(),
					    uncompressed.data(),
					    data.size(),
					    uncomprSize);
  if (decomprSize < 0)
    throw std::runtime_error("uncompress failed");
  return { uncompressed.data(), static_cast<std::size_t>(decomprSize) };
}

} // end of namespace compression
