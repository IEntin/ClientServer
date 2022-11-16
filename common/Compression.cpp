/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Compression.h"
#include "Header.h"
#include "lz4.h"
#include "MemoryPool.h"
#include "Utility.h"

COMPRESSORS Compression::isCompressionEnabled(const std::string& compressorStr) {
  bool enabled = compressorStr.starts_with(LZ4);
  COMPRESSORS compressor = enabled ? COMPRESSORS::LZ4 : COMPRESSORS::NONE;
  return compressor;
}

std::string_view Compression::compress(const char* uncompressed, size_t uncompressedSize) {
  std::vector<char>& buffer = MemoryPool::instance().getFirstBuffer(LZ4_compressBound(uncompressedSize));
  size_t compressedSize = LZ4_compress_default(uncompressed,
					       buffer.data(),
					       uncompressedSize,
					       buffer.capacity());
  if (compressedSize == 0) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':'
	 << (errno ? strerror(errno) : "failed") << std::endl;
    throw std::runtime_error(std::strerror(errno));
  }
  return { buffer.data(), compressedSize };
}

std::string_view Compression::uncompress(const char* compressed,
					 size_t compressedSize,
					 size_t uncomprSize) {
  std::vector<char>& uncompressed = MemoryPool::instance().getFirstBuffer(uncomprSize);
  ssize_t decomprSize = LZ4_decompress_safe(compressed,
					    uncompressed.data(),
					    compressedSize,
					    uncomprSize);
  if (decomprSize != static_cast<ssize_t>(uncomprSize)) {
    if (errno)
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << strerror(errno) << std::endl;
    return { 0, 0 };
  }
  return { uncompressed.data(), uncomprSize };
}

bool Compression::uncompress(const std::vector<char>& compressed,
			     size_t compressedSize,
			     std::vector<char>& uncompressed) {
  ssize_t decomprSize = LZ4_decompress_safe(compressed.data(),
					    uncompressed.data(),
					    compressedSize,
					    uncompressed.size());
  if (decomprSize != static_cast<ssize_t>(uncompressed.size())) {
    if (errno)
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << strerror(errno) << std::endl;
    return false;
  }
  return true;
}

int Compression::getCompressBound(int uncomprSize) {
  return LZ4_compressBound(uncomprSize);
}
