/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Compression.h"
#include "Header.h"
#include "Logger.h"
#include "lz4.h"
#include <cstring>

COMPRESSORS Compression::isCompressionEnabled(const std::string& compressorStr) {
  bool enabled = compressorStr.starts_with(LZ4);
  COMPRESSORS compressor = enabled ? COMPRESSORS::LZ4 : COMPRESSORS::NONE;
  return compressor;
}

std::string_view Compression::compress(const char* uncompressed, size_t uncompressedSize) {
  static thread_local std::vector<char> buffer;
  buffer.clear();
  buffer.reserve(LZ4_compressBound(uncompressedSize));
  size_t compressedSize = LZ4_compress_default(uncompressed,
					       buffer.data(),
					       uncompressedSize,
					       buffer.capacity());
  if (compressedSize == 0) {
    LogError << (errno ? strerror(errno) : "failed") << std::endl;
    throw std::runtime_error("compress failed");
  }
  return { buffer.data(), compressedSize };
}

void Compression::uncompress(const std::vector<char>& compressed, std::vector<char>& uncompressed) {
  ssize_t decomprSize = LZ4_decompress_safe(compressed.data(),
					    uncompressed.data(),
					    compressed.size(),
					    uncompressed.size());
  if (decomprSize != static_cast<ssize_t>(uncompressed.size()))
    throw std::runtime_error("uncompress failed");
}

int Compression::getCompressBound(int uncomprSize) {
  return LZ4_compressBound(uncomprSize);
}
