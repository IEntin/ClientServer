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
#include "Utility.h"
#include <fcntl.h>
#include <filesystem>
#include <sys/types.h>
#include <sys/stat.h>

namespace fifo {

FifoSession::FifoSession(const ServerOptions& options, std::string_view clientId) :
  RunnableT(options._maxFifoSessions),
  _options(options),
  _clientId(clientId) {
  TaskController::totalSessions()++;
  _fifoName.append(_options._fifoDirectoryName).append(1,'/').append(clientId);
  Logger(LOG_LEVEL::DEBUG) << __FILE__ << ':' << __LINE__ << ' ' << __func__
			   << "-_fifoName:" << _fifoName << std::endl;
}

FifoSession::~FifoSession() {
  TaskController::totalSessions()--;
  std::filesystem::remove(_fifoName);
  Logger(LOG_LEVEL::TRACE) << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

void FifoSession::run() {
  CountRunning countRunning;
  if (!std::filesystem::exists(_fifoName))
    // client is closed
    return;
  while (!_stopped) {
    HEADER header;
    _uncompressedRequest.clear();
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
  Runnable::checkCapacity();
  Logger(LOG_LEVEL::INFO) << __FILE__ << ':' << __LINE__ << ' ' << __func__
    << " total sessions=" << TaskController::totalSessions()
    << " fifo sessions="  << _numberObjects
    << ",running=" << _numberRunning << std::endl;
  if (_status == STATUS::MAX_SPECIFIC_SESSIONS)
    Logger(LOG_LEVEL::WARN) << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << "\nThe number of running fifo clients=" << _numberRunning
	 << " is at thread pool capacity." << std::endl;
}

bool FifoSession::start() {
  if (mkfifo(_fifoName.data(), 0620) == -1 && errno != EEXIST) {
    Error() << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	    << std::strerror(errno) << '-' << _fifoName << std::endl;
    return false;
  }
  checkCapacity();
  if (!sendStatusToClient())
    return false;
  return true;
}

void FifoSession::stop() {
  _stopped = true;
  Fifo::onExit(_fifoName, _options._numberRepeatENXIO, _options._ENXIOwait);
}

bool FifoSession::receiveRequest(std::vector<char>& message, HEADER& header) {
  if (_status == STATUS::MAX_SPECIFIC_SESSIONS)
    _status = STATUS::NONE;
  int fdRead = -1;
  utility::CloseFileDescriptor cfdr(fdRead);
  fdRead = open(_fifoName.data(), O_RDONLY);
  if (fdRead == -1) {
    Error() << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	    << std::strerror(errno) << ' ' << _fifoName << std::endl;
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
    if (fdWrite == -1 && (errno == ENXIO || errno == EINTR))
      std::this_thread::sleep_for(std::chrono::milliseconds(_options._ENXIOwait));
  } while (fdWrite == -1 &&
	   (errno == ENXIO || errno == EINTR) && rep++ < _options._numberRepeatENXIO);
  if (fdWrite == -1) {
    Error() << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	    << std::strerror(errno) << ' ' << _fifoName << std::endl;
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
    Error() << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	    << std::strerror(errno) << ' ' << _options._acceptorName << std::endl;
    return false;
  }
  size_t size = _clientId.size();
  std::vector<char> buffer(HEADER_SIZE + size);
  encodeHeader(buffer.data(), HEADERTYPE::CREATE_SESSION, size, size, COMPRESSORS::NONE, false, _status);
  std::copy(_clientId.cbegin(), _clientId.cend(), buffer.begin() + HEADER_SIZE);
  if (!Fifo::writeString(fd, std::string_view(buffer.data(), HEADER_SIZE + size)))
    Error() << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ": failed." << std::endl;
  return true;
}

} // end of namespace fifo
