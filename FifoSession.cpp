/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoSession.h"
#include "Compression.h"
#include "Fifo.h"
#include "MemoryPool.h"
#include "ServerOptions.h"
#include "ServerUtility.h"
#include "TaskController.h"
#include "ThreadPoolDiffObj.h"
#include "Utility.h"
#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>

namespace fifo {

FifoSession::FifoSession(const ServerOptions& options,
			 std::string_view clientId,
			 ThreadPoolDiffObj& threadPool) :
  RunnableT(options._maxFifoSessions),
  _options(options),
  _clientId(clientId),
  _fifoName(_options._fifoDirectoryName + '/' + _clientId),
  _threadPool(threadPool) {
  Debug << "_fifoName:" << _fifoName << std::endl;
}

FifoSession::~FifoSession() {
 std::filesystem::remove(_fifoName);
 Trace << std::endl;
}

void FifoSession::run() {
  CountRunning countRunning;
  if (!std::filesystem::exists(_fifoName))
    return;
  while (!_stopped) {
    _uncompressedRequest.clear();
    HEADER header;
    if (!receiveRequest(_uncompressedRequest, header))
      break;
    static thread_local Response response;
    response.clear();
    auto weakPtr = TaskController::weakInstance();
    auto taskController = weakPtr.lock();
    if (taskController)
      taskController->processTask(header, _uncompressedRequest, response);
    else
      break;
    if (!sendResponse(response))
      break;
  }
}

void FifoSession::checkCapacity() {
  Info << "Number fifo clients=" << _numberObjects
       << ", max number fifo running=" << _maxNumberRunningByType
       << std::endl;
  switch (_status) {
  case STATUS::MAX_SPECIFIC_OBJECTS:
    Warn << "\nThe number of fifo clients=" << _numberObjects
	 << " exceeds thread pool capacity." << std::endl;
    break;
  case STATUS::MAX_TOTAL_OBJECTS:
    Warn << "\nTotal clients=" << _threadPool.numberRelatedObjects()
	 << " exceeds system capacity." << std::endl;
    break;
  default:
    break;
  }
}

bool FifoSession::start() {
  if (mkfifo(_fifoName.data(), 0666) == -1 && errno != EEXIST) {
    LogError << std::strerror(errno) << '-' << _fifoName << std::endl;
    return false;
  }
  _threadPool.push(shared_from_this());
  return true;
}

void FifoSession::stop() {
  _stopped = true;
  Fifo::onExit(_fifoName, _options);
}

bool FifoSession::receiveRequest(std::vector<char>& message, HEADER& header) {
  switch (_status) {
  case STATUS::MAX_SPECIFIC_OBJECTS:
  case STATUS::MAX_TOTAL_OBJECTS:
    _status = STATUS::NONE;
    break;
  default:
    break;
  }
  int fdRead = -1;
  utility::CloseFileDescriptor cfdr(fdRead);
  fdRead = open(_fifoName.data(), O_RDONLY);
  if (fdRead == -1) {
    LogError << std::strerror(errno) << ' ' << _fifoName << std::endl;
    return false;
  }
  return serverutility::receiveRequest(fdRead, message, header, _options);
}

bool FifoSession::sendResponse(const Response& response) {
  HEADER header;
  std::string_view body =
    serverutility::buildReply(response, header, _options._compressor, _status);
  if (body.empty())
    return false;
  // Open write fd in NONBLOCK mode in order to protect the server
  // from a client crashed or killed with SIGKILL and resume operation
  // after a client restarted (in block mode the server will just hang on
  // open(...) no matter what).
  int fdWrite = -1;
  utility::CloseFileDescriptor cfdw(fdWrite);
  fdWrite = Fifo::openWriteEndNonBlock(_fifoName, _options);
  if (fdWrite == -1) {
    LogError << std::strerror(errno) << ' ' << _fifoName << std::endl;
    MemoryPool::destroyBuffers();
    return false;
  }
  if (_options._setPipeSize)
    Fifo::setPipeSize(fdWrite, body.size());
  return Fifo::sendMsg(fdWrite, header, body);
}

bool FifoSession::sendStatusToClient() {
  int fd = -1;
  utility::CloseFileDescriptor closeFd(fd);
  fd = open(_options._acceptorName.data(), O_WRONLY);
  if (fd == -1) {
    LogError << std::strerror(errno) << ' ' << _fifoName << std::endl;
    return false;
  }
  size_t size = _clientId.size();
  HEADER header{ HEADERTYPE::CREATE_SESSION, size, size, COMPRESSORS::NONE, false, _status };
  return Fifo::sendMsg(fd, header, _clientId);
}

} // end of namespace fifo
