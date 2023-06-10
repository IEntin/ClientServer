/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ServerUtility.h"
#include "Compression.h"
#include "Logger.h"
#include "MemoryPool.h"

namespace serverutility {

std::string_view buildReply(const Response& response,
			    HEADER& header,
			    COMPRESSORS compressor,
			    bool encrypted,
			    STATUS status) {
  if (response.empty())
    return {};
  bool bcompressed = compressor != COMPRESSORS::NONE;
  static auto& printOnce[[maybe_unused]] = Logger(LOG_LEVEL::DEBUG)
    << "compression " << (bcompressed ? "enabled." : "disabled.") << std::endl;
  size_t uncomprSize = 0;
  for (const auto& entry : response)
    uncomprSize += entry.size();
  std::vector<char>& buffer = MemoryPool::instance().getSecondBuffer(uncomprSize);
  buffer.resize(uncomprSize);
  ssize_t pos = 0;
  for (const auto& entry : response) {
    std::copy(entry.cbegin(), entry.cend(), buffer.begin() + pos);
    pos += entry.size();
  }
  if (bcompressed) {
    try {
      std::string_view compressedView = Compression::compress(buffer.data(), uncomprSize);
      header = { HEADERTYPE::SESSION, uncomprSize, compressedView.size(), compressor, encrypted, false, status };
      return compressedView;
    }
    catch (const std::exception& e) {
      LogError << e.what() << std::endl;
      return {};
    }
  }
  else
    header = { HEADERTYPE::SESSION, uncomprSize, uncomprSize, compressor, encrypted, false, status };
  return { buffer.data(), buffer.size() };
}

} // end of namespace serverutility
