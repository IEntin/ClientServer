/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Compression.h"

#include <lz4.h>
#include <stdexcept>
#include <vector>

#include "Logger.h"

namespace compression {

void compress(std::string& data) {
  static thread_local std::vector<char> buffer;
  //LogAlways << "\t### " << buffer.capacity() << '\n';
  buffer.reserve(LZ4_compressBound(data.size()));
  std::size_t compressedSize = LZ4_compress_default(data.data(),
						    buffer.data(),
						    data.size(),
						    buffer.capacity());
  if (compressedSize == 0)
    throw std::runtime_error("compress failed");
  data.assign(buffer.data(), compressedSize);
}

void uncompress(std::string& data, std::size_t uncomprSize) {
  static thread_local std::vector<char> uncompressed;
  //LogAlways << "\t### " << uncompressed.capacity() << '\n';
  uncompressed.reserve(uncomprSize);
  ssize_t decomprSize = LZ4_decompress_safe(data.data(),
					    uncompressed.data(),
					    data.size(),
					    uncomprSize);
  if (decomprSize < 0)
    throw std::runtime_error("uncompress failed");
  data.assign(uncompressed.data(), static_cast<size_t>(decomprSize));
}

} // end of namespace compression
