/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ServerUtility.h"
#include "Compression.h"
#include "Fifo.h"
#include "Logger.h"
#include "MemoryPool.h"
#include "ServerOptions.h"

namespace serverutility {

std::string_view buildReply(const Response& response, HEADER& header, COMPRESSORS compressor, STATUS status) {
  if (response.empty())
    return std::string_view();
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
      buffer.resize(compressedView.size());
      header = { HEADERTYPE::SESSION, uncomprSize, compressedView.size(), compressor, false, status };
      return std::string_view(compressedView.data(), compressedView.size());
    }
    catch (const std::exception& e) {
      LogError << e.what() << std::endl;
      return std::string_view();
    }
  }
  else
    header = { HEADERTYPE::SESSION, uncomprSize, uncomprSize, compressor, false, status };
  return std::string_view(buffer.cbegin(), buffer.cend());
}

} // end of namespace serverutility
