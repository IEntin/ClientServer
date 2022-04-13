#include "ServerUtility.h"
#include "Compression.h"
#include "Header.h"
#include "MemoryPool.h"
#include <cassert>

namespace serverutility {

std::string_view buildReply(const Batch& batch, COMPRESSORS compressor, MemoryPool& memoryPool) {
  if (batch.empty())
    return std::string_view();
  bool bcompressed = compressor == COMPRESSORS::LZ4;
  static auto& printOnce[[maybe_unused]] =
    std::clog << LZ4 << "compression " << (bcompressed ? "enabled" : "disabled") << std::endl;
  size_t uncomprSize = 0;
  for (const auto& chunk : batch)
    uncomprSize += chunk.size();
  size_t requestedSize = uncomprSize + HEADER_SIZE;
  std::vector<char>& buffer = memoryPool.getSecondaryBuffer(requestedSize);
  buffer.resize(uncomprSize + HEADER_SIZE);
  size_t pos = HEADER_SIZE;
  for (const auto& chunk : batch) {
    std::copy(chunk.cbegin(), chunk.cend(), buffer.begin() + pos);
    pos += chunk.size();
  }
  std::string_view uncompressedView(buffer.data() + HEADER_SIZE, uncomprSize);
  if (bcompressed) {
    std::string_view dstView = Compression::compress(uncompressedView, memoryPool);
    if (dstView.empty())
      return std::string_view();
    buffer.resize(HEADER_SIZE + dstView.size());
    encodeHeader(buffer.data(), uncomprSize, dstView.size(), compressor, false);
    std::copy(dstView.cbegin(), dstView.cend(), buffer.begin() + HEADER_SIZE);
  }
  else
    encodeHeader(buffer.data(), uncomprSize, uncomprSize, compressor, false);
  std::string_view sendView(buffer.cbegin(), buffer.cend());
  return sendView;
}

} // end of namespace serverutility
