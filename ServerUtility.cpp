#include "ServerUtility.h"
#include "Header.h"
#include "MemoryPool.h"
#include <cassert>
#include <iostream>

namespace serverutility {

std::string_view buildReply(const Batch& batch, const CompressionType& compression) {
  static std::string_view empty;
  if (batch.empty())
    return empty;
  const auto[compressor, enabled] = compression;
  static auto& printOnce[[maybe_unused]] =
    std::clog << LZ4 << "compression " << (enabled ? "enabled" : "disabled") << std::endl;
  size_t uncomprSize = 0;
  for (const auto& chunk : batch)
    uncomprSize += chunk.size();
  size_t requestedSize = Compression::getCompressBound(uncomprSize) + HEADER_SIZE;
  std::vector<char>& buffer = MemoryPool::getSecondaryBuffer(requestedSize);
  buffer.resize(uncomprSize + HEADER_SIZE);
  size_t pos = HEADER_SIZE;
  for (const auto& chunk : batch) {
    std::copy(chunk.cbegin(), chunk.cend(), buffer.begin() + pos);
    pos += chunk.size();
  }
  std::string_view uncompressedView(buffer.data() + HEADER_SIZE, uncomprSize);
  if (enabled) {
    std::string_view dstView = Compression::compress(uncompressedView);
    if (dstView.empty())
      return empty;
    buffer.resize(HEADER_SIZE + dstView.size());
    encodeHeader(buffer.data(), uncomprSize, dstView.size(), compressor, false);
    std::copy(dstView.cbegin(), dstView.cend(), buffer.begin() + HEADER_SIZE);
  }
  else {
    assert(compressor == COMPRESSORS::NONE);
    encodeHeader(buffer.data(), uncomprSize, uncomprSize, compressor, false);
  }
  std::string_view sendView(buffer.cbegin(), buffer.cend());
  return sendView;
}

} // end of namespace commutility
