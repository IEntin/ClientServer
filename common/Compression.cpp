/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Compression.h"
#include "Header.h"
#include "lz4.h"
#include <cassert>
#include <cstring>
#include <iostream>

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
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << strerror(errno) << std::endl;
    return { nullptr, 0 };
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
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << strerror(errno) << std::endl;
    return false;
  }
  return true;
}

size_t Compression::getCompressBound(size_t uncomprSize) {
  return LZ4_compressBound(uncomprSize);
}

std::string_view Compression::compress(std::string_view origin, MemoryPool& memoryPool) {
  size_t needCapacity = LZ4_compressBound(origin.size());
  size_t dstCapacity = 0;
  if (memoryPool.getPrimaryBuffer().capacity() < needCapacity)
    // need to increase
    dstCapacity = origin.size();
  else
    // can use as is
    dstCapacity = 0;
  std::vector<char>& buffer = memoryPool.getPrimaryBuffer(dstCapacity);
  std::string_view dstView = compressInternal(origin, buffer.data(), needCapacity);
  if (dstView.empty()) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ":failed to compress payload" << std::endl;
    return { nullptr, 0 };
  }
  return dstView;
}

std::string_view Compression::uncompress(std::string_view compressed,
					 size_t uncomprSize,
					 MemoryPool& memoryPool) {
  std::vector<char>& buffer = memoryPool.getPrimaryBuffer(uncomprSize);
  if (!uncompressInternal(compressed, buffer.data(), uncomprSize)) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << " failed" << std::endl;
    return { nullptr, 0 };
  }
  return { buffer.data(), uncomprSize };
}

bool Compression::uncompress(std::string_view compressed, std::vector<char>& uncompressed) {
  if (!uncompressInternal(compressed, uncompressed.data(), uncompressed.size())) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << " failed" << std::endl;
    return false;
  }
  return true;
}
