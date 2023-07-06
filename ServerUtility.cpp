/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ServerUtility.h"
#include "CommonUtils.h"
#include "Options.h"
#include "TaskController.h"

namespace serverutility {

std::string_view buildReply(const Options&options,
			    const CryptoKeys& keys,
			    const Response& response,
			    HEADER& header,
			    STATUS status) {
  if (response.empty())
    return {};
  size_t dataSize = 0;
  for (const auto& entry : response)
    dataSize += entry.size();
  static thread_local std::string data;
  data.clear();
  data.resize(dataSize);
  ssize_t pos = 0;
  for (const auto& entry : response) {
    std::copy(entry.cbegin(), entry.cend(), data.begin() + pos);
    pos += entry.size();
  }
  commonutils::compressEncrypt(options, keys, data,  header, false, status);
  return data;
}

bool processRequest(const CryptoKeys& keys,
		    const HEADER& header,
		    const std::vector<char>& received,
		    Response& response) {
  std::string_view receivedView(received.data(), received.size());
  std::string_view decryptedView = commonutils::decryptDecompress(keys, header, receivedView);
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
