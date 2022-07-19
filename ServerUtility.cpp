/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ServerUtility.h"
#include "Compression.h"
#include "Fifo.h"
#include "MemoryPool.h"
#include "ServerOptions.h"
#include "Utility.h"
#include <cassert>

namespace serverutility {

std::string_view buildReply(const Response& response, COMPRESSORS compressor, unsigned short ephemeral) {
  if (response.empty())
    return std::string_view();
  bool bcompressed = compressor == COMPRESSORS::LZ4;
  static auto& printOnce[[maybe_unused]] =
    CLOG << LZ4 << "compression " << (bcompressed ? "enabled\n" : "disabled.\n");
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
    if (dstView.empty())
      return std::string_view();
    buffer.resize(HEADER_SIZE + dstView.size());
    encodeHeader(buffer.data(), uncomprSize, dstView.size(), compressor, false, ephemeral, 'R');
    std::copy(dstView.begin(), dstView.end(), buffer.begin() + HEADER_SIZE);
  }
  else
    encodeHeader(buffer.data(), uncomprSize, uncomprSize, compressor, false, ephemeral, 'R');
  std::string_view sendView(buffer.cbegin(), buffer.cend());
  return sendView;
}

bool readMsgBody(int fd,
		 HEADER header,
		 std::vector<char>& uncompressed,
		 const ServerOptions& options) {
  const auto& [uncomprSize, comprSize, compressor, diagnostics, ephemeral, clientId, problem] = header;
  bool bcompressed = compressor == COMPRESSORS::LZ4;
  static auto& printOnce[[maybe_unused]] =
    CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << (bcompressed ? " received compressed" : " received not compressed") << '\n';
  if (bcompressed) {
    std::vector<char>& buffer = MemoryPool::instance().getFirstBuffer(comprSize);
    if (!fifo::Fifo::readString(fd, buffer.data(), comprSize, options._numberRepeatEINTR))
      return false;
    std::string_view received(buffer.data(), comprSize);
    uncompressed.resize(uncomprSize);
    if (!Compression::uncompress(received, uncompressed)) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	   << ":failed to uncompress payload.\n";
      return false;
    }
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
    if (options._destroyBufferOnClientDisconnect)
      MemoryPool::destroyBuffers();
    return false;
  }
  return readMsgBody(fd, header, message, options);
}

} // end of namespace serverutility
