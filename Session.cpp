/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Session.h"

#include "Server.h"
#include "ServerOptions.h"
#include "Task.h"
#include "TaskController.h"
#include "Utility.h"

#ifdef CRYPTOVARIANT
#include "CryptoVariant.h"
#endif

Session::Session(ServerWeakPtr server,
		 std::string_view encodedPeerPubKeyAes,
		 std::string_view signatureWithPubKey)
try :
  _task(std::make_shared<Task>(server)),
  _server(server) {
    _clientId = utility::getUniqueId();
#ifdef CRYPTOVARIANT
    _encryptorContainer = cryptovariant::createCrypto(encodedPeerPubKeyAes, signatureWithPubKey);
#elifdef CRYPTOTUPLE
    _encryptorContainer = std::make_tuple(std::make_shared<CryptoPlPl>(encodedPeerPubKeyAes, signatureWithPubKey),
					  std::make_shared<CryptoSodium>(encodedPeerPubKeyAes, signatureWithPubKey));
#endif
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }

std::pair<HEADER, std::string_view>
Session::buildReply(std::atomic<STATUS>& status) {
  _responseData.clear();
  const auto& response = _task->getResponse();
  for (std::string_view entry : response)
    _responseData += entry;
  HEADER header =
    { HEADERTYPE::SESSION, _responseData.size(), 0,
      ServerOptions::_compressor, DIAGNOSTICS::NONE, status, 0 };
  std::string_view dataView =
  cryptocommon::compressEncrypt(_encryptorContainer,
				_buffer,
				header,
				_responseData,
				ServerOptions::_doEncrypt,
				ServerOptions::_compressionLevel);
  header = { HEADERTYPE::SESSION, dataView.size(), 0,
	     ServerOptions::_compressor, DIAGNOSTICS::NONE, status, 0 };
  return { header, dataView };
}

bool Session::processTask() {
  cryptocommon::decryptDecompress(_encryptorContainer, _buffer, _header, _request);
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
				   STATUS status) {
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
