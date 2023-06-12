/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ServerUtility.h"
#include "Compression.h"
#include "Encryption.h"
#include "Logger.h"
#include "MemoryPool.h"
#include "TaskController.h"

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

bool processRequest(const HEADER& header, std::vector<char>& request, Response& response) {
  static thread_local std::vector<char> uncompressed;
  uncompressed.clear();
  bool bcompressed = isCompressed(header);
  if (bcompressed) {
    static auto& printOnce[[maybe_unused]] = Trace << " received compressed." << std::endl;
    size_t uncompressedSize = extractUncompressedSize(header);
    uncompressed.resize(uncompressedSize);
    if (!Compression::uncompress(request, request.size(), uncompressed)) {
      LogError << "decompression failed." << std::endl;
      return false;
    }
  }
  else
    static auto& printOnce[[maybe_unused]] = Trace << " received not compressed." << std::endl;
  std::string_view requestView(request.data(), request.size());
  std::string_view uncompressedView(uncompressed.data(), uncompressed.size());
  /*
  bool bencrypted = isEncrypted(header);
  static thread_local std::string decrypted;
  if (!Encryption::decrypt(uncompressedView, Encryption::getKey(), Encryption::getIv(), decrypted))
    return false;
  */
  auto weakPtr = TaskController::weakInstance();
  auto taskController = weakPtr.lock();
  if (taskController) {
    taskController->processTask(header, (bcompressed ? uncompressedView : requestView), response);
    return true;
  }
  return false;
}

} // end of namespace serverutility
