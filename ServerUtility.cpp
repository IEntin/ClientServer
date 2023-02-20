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

bool readMsgBody(int fd,
		 HEADER header,
		 std::vector<char>& uncompressed,
		 const ServerOptions& options) {
  const auto& [headerType, uncomprSize, comprSize, compressor, diagnostics, status] = header;
  bool bcompressed = isCompressed(header);
  static auto& printOnce[[maybe_unused]] =
    Debug
	 << (bcompressed ? " received compressed" : " received not compressed") << std::endl;
  if (bcompressed) {
    std::vector<char>& buffer = MemoryPool::instance().getFirstBuffer(comprSize);
    if (!fifo::Fifo::readString(fd, buffer.data(), comprSize, options))
      return false;
    uncompressed.resize(uncomprSize);
    if (!Compression::uncompress(buffer, comprSize, uncompressed))
      return false;
  }
  else {
    uncompressed.resize(uncomprSize);
    if (!fifo::Fifo::readString(fd, uncompressed.data(), uncomprSize, options))
      return false;
  }
  return true;
}

bool receiveRequest(int fd, std::vector<char>& message, HEADER& header, const ServerOptions& options) {
  try {
    header = fifo::Fifo::readHeader(fd, options);
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
    MemoryPool::destroyBuffers();
    return false;
  }
  return readMsgBody(fd, header, message, options);
}

} // end of namespace serverutility
