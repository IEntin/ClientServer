#include "CommUtility.h"
#include "Compression.h"
#include "Header.h"
#include "MemoryPool.h"
#include "Utility.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

namespace commutility {

std::string_view buildReply(const Batch& batch, const std::pair<COMPRESSORS, bool>& compression) {
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
  else
    encodeHeader(buffer.data(), uncomprSize, uncomprSize, COMPRESSORS::NONE, false);
  std::string_view sendView(buffer.cbegin(), buffer.cend());
  return sendView;
}

size_t createPayload(const char* sourceName, Batch& payload) {
  std::ifstream input(sourceName, std::ifstream::in | std::ifstream::binary);
  if (!input)
    throw std::runtime_error(sourceName);
  unsigned long long requestIndex = 0;
  std::string line;
  Batch batch;
  while (std::getline(input, line)) {
    if (line.empty())
      continue;
    std::string modifiedLine(utility::createRequestId(requestIndex++));
    modifiedLine.append(line.append(1, '\n'));
    payload.emplace_back(std::move(modifiedLine));
  }
  return payload.size();
}

std::string readFileContent(const std::string& name) {
  std::ifstream ifs(name, std::ifstream::in | std::ifstream::binary);
  if (!ifs) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':'
	      << std::strerror(errno) << ' ' << name << std::endl;
    return "";
  }
  std::stringstream buffer;
  buffer << ifs.rdbuf();
  return buffer.str();
}

} // end of namespace commutility
