/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Compression.h"
#include "Header.h"
#include "MemoryPool.h"
#include "lz4.h"
#include <cassert>
#include <cstring>
#include <iostream>

namespace {

constexpr std::string_view LZ4 = "LZ4";

} // end of anonimous namespace

COMPRESSORS Compression::_compressor = COMPRESSORS::NONE;
bool Compression::_enabled = false;

std::pair<COMPRESSORS, bool> Compression::isCompressionEnabled() {
  return std::make_pair(_compressor, _enabled);
}

bool Compression::setCompressionEnabled(const std::string& compressorStr) {
  _enabled = compressorStr.starts_with(LZ4);
  _compressor = COMPRESSORS::LZ4;
  std::clog << LZ4 << "compression " << (_enabled ? "enabled" : "disabled") << std::endl;
  return _enabled;
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

std::string_view Compression::compress(std::string_view origin) {
  size_t dstCapacity = LZ4_compressBound(origin.size());
  auto[buffer, size] = MemoryPool::getPrimaryBuffer(dstCapacity);
  assert(buffer);
  std::string_view dstView = compressInternal(origin, buffer, dstCapacity);
  if (dstView.empty()) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ":failed to compress payload" << std::endl;
    return { nullptr, 0 };
  }
  return dstView;
}

std::string_view Compression::uncompress(std::string_view compressed, size_t uncomprSize) {
  auto[buffer, size] = MemoryPool::getPrimaryBuffer(uncomprSize);
  assert(buffer);
  if (!uncompressInternal(compressed, buffer, uncomprSize)) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << " failed" << std::endl;
    return { nullptr, 0 };
  }
  return { buffer, uncomprSize };
}

bool Compression::uncompress(std::string_view compressed, std::vector<char>& uncompressed) {
  if (!uncompressInternal(compressed, uncompressed.data(), uncompressed.size())) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << " failed" << std::endl;
    return false;
  }
  return true;
}
