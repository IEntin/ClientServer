/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ServerUtility.h"
#include "Compression.h"
#include "Header.h"
#include "MemoryPool.h"
#include "Utility.h"
#include <cassert>
#include <iostream>

namespace serverutility {

std::string_view buildReply(const Response& response, COMPRESSORS compressor, MemoryPool& memoryPool) {
  if (response.empty())
    return std::string_view();
  bool bcompressed = compressor == COMPRESSORS::LZ4;
  static auto& printOnce[[maybe_unused]] =
    CLOG << LZ4 << "compression "
      << (bcompressed ? "enabled" : "disabled") << std::endl;
  size_t uncomprSize = 0;
  for (const auto& entry : response)
    uncomprSize += entry.size();
  size_t requestedSize = uncomprSize + HEADER_SIZE;
  thread_local static std::vector<char> buffer(requestedSize);
  buffer.resize(uncomprSize + HEADER_SIZE);
  ssize_t pos = HEADER_SIZE;
  for (const auto& entry : response) {
    std::copy(entry.begin(), entry.end(), buffer.begin() + pos);
    pos += entry.size();
  }
  std::string_view uncompressedView(buffer.data() + HEADER_SIZE, uncomprSize);
  if (bcompressed) {
    std::string_view dstView = Compression::compress(uncompressedView, memoryPool);
    if (dstView.empty())
      return std::string_view();
    buffer.resize(HEADER_SIZE + dstView.size());
    encodeHeader(buffer.data(), uncomprSize, dstView.size(), compressor, false);
    std::copy(dstView.begin(), dstView.end(), buffer.begin() + HEADER_SIZE);
  }
  else
    encodeHeader(buffer.data(), uncomprSize, uncomprSize, compressor, false);
  std::string_view sendView(buffer.cbegin(), buffer.cend());
  return sendView;
}

} // end of namespace serverutility
