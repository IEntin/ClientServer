/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Session.h"

#include "Server.h"
#include "ServerOptions.h"
#include "Task.h"
#include "TaskController.h"
#include "Utility.h"

Session::Session(ServerWeakPtr server,
		 std::span<unsigned char> msgHash,
		 std::span<unsigned char> pubB,
		 std::span<unsigned char> signatureWithPubKey) :
  _task(std::make_shared<Task>(server)),
  _server(server) {
  _clientId = utility::getUniqueId();
  _crypto = createCrypto(msgHash, pubB, signatureWithPubKey);
}

std::pair<HEADER, std::string_view>
Session::buildReply(std::atomic<STATUS>& status) {
  _responseData.clear();
  const auto& response = _task->getResponse();
  for (std::string_view entry : response)
    _responseData += entry;
  if (ServerOptions::_showKey)
    _crypto->showKey();
  HEADER header =
    { HEADERTYPE::SESSION, 0,_responseData.size(), ServerOptions::_encryption,
      ServerOptions::_compressor, DIAGNOSTICS::NONE, status, 0 };
  std::string_view dataView =
    utility::compressEncrypt(_buffer, header, std::weak_ptr(_crypto),
			     _responseData, ServerOptions::_compressionLevel);
  header = { HEADERTYPE::SESSION, 0, dataView.size(), ServerOptions::_encryption,
	     ServerOptions::_compressor, DIAGNOSTICS::NONE, status, 0 };
  return { header, dataView };
}

bool Session::processTask() {
  utility::decryptDecompress(_buffer, _header, std::weak_ptr(_crypto), _request);
  if (auto taskController = TaskController::getWeakPtr().lock(); taskController) {
    _task->update(_header, _request);
    taskController->processTask(_task);
    return true;
  }
  return false;
}

void Session::displayCapacityCheck(std::string_view type,
				   unsigned totalNumberObjects,
				   unsigned numberObjects,
				   unsigned numberRunningByType,
				   unsigned maxNumberRunningByType,
				   STATUS status) const {
  Info << "Number " << type << " sessions=" << numberObjects
       << ", Number running " << type << " sessions=" << numberRunningByType
       << ", max number " << type << " running=" << maxNumberRunningByType
       << '\n';
  switch (status) {
  case STATUS::MAX_OBJECTS_OF_TYPE:
    Warn << "\nThe number of " << type << " sessions="
	 << numberObjects << " exceeds thread pool capacity." << '\n';
    break;
  case STATUS::MAX_TOTAL_OBJECTS:
    Warn << "\nTotal sessions=" << totalNumberObjects
	 << " exceeds system capacity." << '\n';
    break;
  default:
    break;
  }
}
