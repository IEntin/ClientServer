/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoSession.h"
#include "Compression.h"
#include "Fifo.h"
#include "MemoryPool.h"
#include "ServerOptions.h"
#include "ServerUtility.h"
#include "Server.h"
#include "TaskController.h"
#include "Utility.h"
#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>
#include <sys/types.h>

namespace fifo {

FifoSession::FifoSession(const ServerOptions& options,
			 std::string_view clientId,
			 Server& server) :
  RunnableT(options._maxFifoSessions),
  _options(options),
  _server(server),
  _clientId(clientId) {
  _fifoName.append(_options._fifoDirectoryName).append(1,'/').append(clientId);
  Debug << "_fifoName:" << _fifoName << std::endl;
}

FifoSession::~FifoSession() {
  std::filesystem::remove(_fifoName);
  Trace << std::endl;
}

void FifoSession::run() {
  if (!std::filesystem::exists(_fifoName))
    return;
  _status.wait(STATUS::MAX_TOTAL_SESSIONS);
  while (!_stopped) {
    Server::CountRunningSessions countRunning;
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
  unsigned totalSessions = _server.registerSession(shared_from_this());
  Info << "total sessions=" << totalSessions
       << " fifo sessions=" << _numberObjects << std::endl;
  if (_status == STATUS::MAX_TOTAL_SESSIONS) {
    Warn << "\nTotal clients=" << totalSessions
	 << " exceeds system capacity." << std::endl;
    return;
  }
  Runnable::checkCapacity();
  if (_status == STATUS::MAX_SPECIFIC_SESSIONS) {
    Warn << "\nThe number of fifo clients=" << _numberObjects
	 << " exceeds thread pool capacity." << std::endl;
  }
}

bool FifoSession::start() {
  if (mkfifo(_fifoName.data(), 0666) == -1 && errno != EEXIST) {
    LogError << std::strerror(errno) << '-' << _fifoName << std::endl;
    return false;
  }
  checkCapacity();
  return sendStatusToClient();
}

void FifoSession::stop() {
  _server.deregisterSession(weak_from_this());
  STATUS expected = STATUS::MAX_TOTAL_SESSIONS;
  if (_status.compare_exchange_strong(expected, STATUS::NONE)) {
    _status = STATUS::NONE;
    _status.notify_one();
  }
  _stopped = true;
  Fifo::onExit(_fifoName, _options);
}

bool FifoSession::notify() {
  STATUS expected = STATUS::MAX_TOTAL_SESSIONS;
  if (_status.compare_exchange_strong(expected, STATUS::NONE)) {
    _status.notify_one();
    return true;
  }
  return false;
}

bool FifoSession::receiveRequest(std::vector<char>& message, HEADER& header) {
  switch (_status) {
  case STATUS::MAX_SPECIFIC_SESSIONS:
  case STATUS::MAX_TOTAL_SESSIONS:
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
  std::string_view message =
    serverutility::buildReply(response, _options._compressor, _status);
  if (message.empty())
    return false;
  // Open write fd in NONBLOCK mode in order to protect the server
  // from a client crashed or killed with SIGKILL and resume operation
  // after a client restarted (in block mode the server will just hang on
  // open(...) no matter what).
  int fdWrite = -1;
  utility::CloseFileDescriptor cfdw(fdWrite);
  int rep = 0;
  do {
    fdWrite = open(_fifoName.data(), O_WRONLY | O_NONBLOCK);
    if (fdWrite == -1) {
      switch (errno) {
      case ENXIO:
      case EINTR:
	std::this_thread::sleep_for(std::chrono::milliseconds(_options._ENXIOwait));
	break;
      default:
	Info << std::strerror(errno) << ' ' << _fifoName << std::endl;
	MemoryPool::destroyBuffers();
	return false;
	break;
      }
    }
  } while (fdWrite == -1 && rep++ < _options._numberRepeatENXIO);
  if (fdWrite == -1) {
    LogError << std::strerror(errno) << ' ' << _fifoName << std::endl;
    MemoryPool::destroyBuffers();
    return false;
  }
  if (_options._setPipeSize)
    Fifo::setPipeSize(fdWrite, message.size());
  return Fifo::writeString(fdWrite, message);
}

bool FifoSession::sendStatusToClient() {
  int fd = -1;
  utility::CloseFileDescriptor closeFd(fd);
  fd = open(_options._acceptorName.data(), O_WRONLY);
  if (fd == -1) {
    LogError << std::strerror(errno) << ' ' << _options._acceptorName << std::endl;
    return false;
  }
  size_t size = _clientId.size();
  std::vector<char> buffer(HEADER_SIZE + size);
  encodeHeader(buffer.data(), HEADERTYPE::CREATE_SESSION, size, size, COMPRESSORS::NONE, false, _status);
  std::copy(_clientId.cbegin(), _clientId.cend(), buffer.begin() + HEADER_SIZE);
  if (!Fifo::writeString(fd, std::string_view(buffer.data(), HEADER_SIZE + size)))
    LogError << "failed." << std::endl;
  return true;
}

} // end of namespace fifo
