/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ServerUtility.h"
#include "CommonUtils.h"
#include "Logger.h"
#include "Options.h"
#include "TaskController.h"

namespace serverutility {

std::string_view buildReply(const Options&options,
			    const CryptoKeys& keys,
			    const Response& response,
			    HEADER& header,
			    STATUS status) {
  static std::string_view empty;
  if (response.empty())
    return {};
  size_t uncomprSize = 0;
  for (const auto& entry : response)
    uncomprSize += entry.size();
  static thread_local std::vector<char> data;
  data.clear();
  data.resize(uncomprSize);
  ssize_t pos = 0;
  for (const auto& entry : response) {
    std::copy(entry.cbegin(), entry.cend(), data.begin() + pos);
    pos += entry.size();
  }
  std::string_view dataView(data.data(), data.size());
  std::string_view body;
  STATUS result =
    commonutils::encryptCompressData(options, keys, dataView,  header, body, false, status);
  bool failed = false;
  switch (result) {
  case STATUS::ERROR:
  case STATUS::COMPRESSION_PROBLEM:
  case STATUS::ENCRYPTION_PROBLEM:
    failed = true;
    break;
  default:
    break;
  }
  if (failed)
    return empty;
  return { body.data(), body.size() };
}

bool processRequest(const CryptoKeys& keys,
		    const HEADER& header,
		    const std::vector<char>& received,
		    Response& response) {
  std::string_view receivedView(received.data(), received.size());
  std::string_view decryptedView = commonutils::decompressDecrypt(keys, header, receivedView);
  if (decryptedView.empty())
    return false;
  auto weakPtr = TaskController::weakInstance();
  auto taskController = weakPtr.lock();
  if (taskController) {
    taskController->processTask(header, decryptedView, response);
    return true;
  }
  return false;
}

} // end of namespace serverutility
