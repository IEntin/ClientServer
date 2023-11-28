/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ServerUtility.h"
#include "PayloadTransform.h"
#include "ServerOptions.h"
#include "Task.h"
#include "TaskController.h"

namespace serverutility {

std::string_view buildReply(const Response& response,
			    HEADER& header,
			    std::atomic<STATUS>& status) {
  if (response.empty())
    return {};
  header = { HEADERTYPE::SESSION, 0, 0, ServerOptions::_compressor, ServerOptions::_encrypted, false, status };
  static thread_local std::vector<char> data;
  data.resize(0);
  for (const auto& entry : response)
    data.insert(data.end(), entry.begin(), entry.end());
  return payloadtransform::compressEncrypt({ data.data(), data.size() }, header);
}

bool processTask(const HEADER& header, std::string_view input, TaskPtr task) {
  auto weakPtr = TaskController::weakInstance();
  if (auto taskController = weakPtr.lock(); taskController) {
    std::string_view restored = payloadtransform::decryptDecompress(header, input);
    task->initialize(header, restored);
    taskController->processTask(task);
    return true;
  }
  return false;
}

} // end of namespace serverutility
