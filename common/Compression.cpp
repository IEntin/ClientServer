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

std::string_view Compression::compressInternal(std::string_view uncompressed,
					       char* buffer,
					       size_t dstCapacity) {
  size_t compressedSize = LZ4_compress_default(uncompressed.data(),
					       buffer,
					       uncompressed.size(),
					       dstCapacity);
  if (compressedSize == 0) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << strerror(errno) << std::endl;
    throw std::runtime_error(std::string(std::strerror(errno)));
  }
  return { buffer, compressedSize };
}

bool Compression::uncompressInternal(std::string_view compressed,
				     char* uncompressed,
				     size_t uncomprSize) {
  ssize_t decomprSize = LZ4_decompress_safe(compressed.data(),
					    uncompressed,
					    compressed.size(),
					    uncomprSize);
  if (decomprSize != static_cast<ssize_t>(uncomprSize)) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << strerror(errno) << std::endl;
    return false;
  }
  return true;
}

int Compression::getCompressBound(int uncomprSize) {
  return LZ4_compressBound(uncomprSize);
}

std::string_view Compression::compress(std::string_view uncompressed) {
  std::vector<char>& buffer = MemoryPool::instance().getFirstBuffer(LZ4_compressBound(uncompressed.size()));
  std::string_view dstView = compressInternal(uncompressed, buffer.data(), buffer.capacity());
  if (dstView.empty()) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << ":failed to compress payload" << '\n';
    throw std::runtime_error("Failed to compress payload.");
  }
  return dstView;
}

std::string_view Compression::uncompress(std::string_view compressed, size_t uncomprSize) {
  std::vector<char>& buffer = MemoryPool::instance().getFirstBuffer(uncomprSize);
  if (!uncompressInternal(compressed, buffer.data(), uncomprSize)) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << " failed" << '\n';
    throw std::runtime_error("Failed to uncompress string.");
  }
  return { buffer.data(), uncomprSize };
}

void Compression::uncompress(std::string_view compressed, std::vector<char>& uncompressed) {
  if (!uncompressInternal(compressed, uncompressed.data(), uncompressed.size())) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << " failed" << '\n';
    throw std::runtime_error("Failed to uncompress string.");
  }
}
