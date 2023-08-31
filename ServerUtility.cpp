/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ServerUtility.h"
#include "PayloadTransform.h"
#include "TaskController.h"

namespace serverutility {

std::string_view buildReply(const Response& response, HEADER& header) {
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
  payloadtransform::compressEncrypt(data, header);
  return data;
}

bool processRequest(const HEADER& header, std::string& received, Response& response) {
  payloadtransform::decryptDecompress(header, received);
  auto weakPtr = TaskController::weakInstance();
  auto taskController = weakPtr.lock();
  if (taskController) {
    taskController->processTask(header, received, response);
    return true;
  }
  return false;
}

} // end of namespace serverutility
