/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ServerUtility.h"
#include "Compression.h"
#include "Fifo.h"
#include "MemoryPool.h"
#include "ServerOptions.h"
#include "Utility.h"

namespace serverutility {

std::string_view buildReply(const Response& response, COMPRESSORS compressor, PROBLEMS problem) {
  if (response.empty())
    return std::string_view();
  bool bcompressed = compressor == COMPRESSORS::LZ4;
  static auto& printOnce[[maybe_unused]] =
    CLOG << LZ4 << "compression " << (bcompressed ? "enabled." : "disabled.") << std::endl;
  size_t uncomprSize = 0;
  for (const auto& entry : response)
    uncomprSize += entry.size();
  size_t requestedSize = uncomprSize + HEADER_SIZE;
  std::vector<char>& buffer = MemoryPool::instance().getSecondBuffer(requestedSize);
  buffer.resize(requestedSize);
  ssize_t pos = HEADER_SIZE;
  for (const auto& entry : response) {
    std::copy(entry.begin(), entry.end(), buffer.begin() + pos);
    pos += entry.size();
  }
  std::string_view uncompressedView(buffer.data() + HEADER_SIZE, uncomprSize);
  if (bcompressed) {
    std::string_view dstView = Compression::compress(uncompressedView);
    buffer.resize(HEADER_SIZE + dstView.size());
    encodeHeader(buffer.data(),
		 HEADERTYPE::REQUEST,
		 uncomprSize,
		 dstView.size(),
		 compressor,
		 false,
		 problem);
    std::copy(dstView.begin(), dstView.end(), buffer.begin() + HEADER_SIZE);
  }
  else
    encodeHeader(buffer.data(),
		 HEADERTYPE::REQUEST,
		 uncomprSize,
		 uncomprSize,
		 compressor,
		 false,
		 problem);
  std::string_view sendView(buffer.cbegin(), buffer.cend());
  return sendView;
}

bool readMsgBody(int fd,
		 HEADER header,
		 std::vector<char>& uncompressed,
		 const ServerOptions& options) {
  const auto& [headerType, uncomprSize, comprSize, compressor, diagnostics, problem] = header;
  bool bcompressed = compressor == COMPRESSORS::LZ4;
  static auto& printOnce[[maybe_unused]] =
    CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << (bcompressed ? " received compressed" : " received not compressed") << std::endl;
  if (bcompressed) {
    std::vector<char>& buffer = MemoryPool::instance().getFirstBuffer(comprSize);
    if (!fifo::Fifo::readString(fd, buffer.data(), comprSize, options._numberRepeatEINTR))
      return false;
    std::string_view received(buffer.data(), comprSize);
    uncompressed.resize(uncomprSize);
    Compression::uncompress(received, uncompressed);
  }
  else {
    uncompressed.resize(uncomprSize);
    if (!fifo::Fifo::readString(fd, uncompressed.data(), uncomprSize, options._numberRepeatEINTR))
      return false;
  }
  return true;
}

bool receiveRequest(int fd, std::vector<char>& message, HEADER& header, const ServerOptions& options) {
  header = fifo::Fifo::readHeader(fd, options._numberRepeatEINTR);
  if (getProblem(header) != PROBLEMS::NONE) {
    MemoryPool::destroyBuffers();
    return false;
  }
  return readMsgBody(fd, header, message, options);
}

} // end of namespace serverutility
