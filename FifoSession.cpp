/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoSession.h"
#include "Compression.h"
#include "Fifo.h"
#include "FifoAcceptor.h"
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

FifoSession::FifoSession(const ServerOptions& options, std::string_view clientId, FifoAcceptorPtr parent) :
  Runnable(options._maxFifoSessions),
  _options(options),
  _parent(parent) {
  TaskController::totalSessions()++;
  _fifoName.append(_options._fifoDirectoryName).append(1,'/').append(clientId);
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
       << "-_fifoName:" << _fifoName << std::endl;
}

FifoSession::~FifoSession() {
  TaskController::totalSessions()--;
  std::filesystem::remove(_fifoName);
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

void FifoSession::run() {
  if (!std::filesystem::exists(_fifoName))
    // client is closed
    return;
  while (!_parent->_stopped) {
    HEADER header;
    _uncompressedRequest.clear();
    if (!receiveRequest(_uncompressedRequest, header))
      break;
    static thread_local Response response;
    response.clear();
    TaskController::instance()->processTask(header, _uncompressedRequest, response);
    if (!sendResponse(response))
      break;
  }
}

unsigned FifoSession::getNumberObjects() const {
  return _objectCounter._numberObjects;
}

void FifoSession::checkCapacity() {
  Runnable::checkCapacity();
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
       << " total sessions=" << TaskController::totalSessions() << ' '
       << "fifo sessions=" << _objectCounter._numberObjects << std::endl;
  if (_status == STATUS::MAX_NUMBER_RUNNABLES)
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << "\nThe number of fifo clients=" << _objectCounter._numberObjects
	 << " exceeds thread pool capacity." << std::endl;
}

bool FifoSession::start() {
  if (mkfifo(_fifoName.data(), 0620) == -1 && errno != EEXIST) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	 << std::strerror(errno) << '-' << _fifoName << std::endl;
    return false;
  }
  checkCapacity();
  if (!sendStatusToClient())
    return false;
  return true;
}

void FifoSession::stop() {
  Fifo::onExit(_fifoName, _options._numberRepeatENXIO, _options._ENXIOwait);
  _parent->remove(shared_from_this());
}

bool FifoSession::receiveRequest(std::vector<char>& message, HEADER& header) {
  if (_status == STATUS::MAX_NUMBER_RUNNABLES)
    _status.store(STATUS::NONE);
  utility::CloseFileDescriptor cfdr(_fdRead);
  _fdRead = open(_fifoName.data(), O_RDONLY);
  if (_fdRead == -1) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-' 
	 << std::strerror(errno) << ' ' << _fifoName << std::endl;
    return false;
  }
  return serverutility::receiveRequest(_fdRead, message, header, _options);
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
  utility::CloseFileDescriptor cfdw(_fdWrite);
  int rep = 0;
  do {
    _fdWrite = open(_fifoName.data(), O_WRONLY | O_NONBLOCK);
    if (_fdWrite == -1 && (errno == ENXIO || errno == EINTR))
      std::this_thread::sleep_for(std::chrono::milliseconds(_options._ENXIOwait));
  } while (_fdWrite == -1 &&
	   (errno == ENXIO || errno == EINTR) && rep++ < _options._numberRepeatENXIO);
  if (_fdWrite == -1) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	 << std::strerror(errno) << ' ' << _fifoName << std::endl;
    MemoryPool::destroyBuffers();
    return false;
  }
  if (_options._setPipeSize)
    Fifo::setPipeSize(_fdWrite, message.size());
  return Fifo::writeString(_fdWrite, message);
}

bool FifoSession::sendStatusToClient() {
  int fd = -1;
  utility::CloseFileDescriptor closeFd(fd);
  fd = open(_options._acceptorName.data(), O_WRONLY);
  if (fd == -1) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	 << std::strerror(errno) << ' ' << _options._acceptorName << std::endl;
    return false;
  }
  size_t size = _fifoName.size();
  std::vector<char> buffer(HEADER_SIZE + size);
  encodeHeader(buffer.data(), HEADERTYPE::SESSION, size, size, COMPRESSORS::NONE, false, _status);
  std::copy(_fifoName.begin(), _fifoName.end(), buffer.begin() + HEADER_SIZE);
  if (!Fifo::writeString(fd, std::string_view(buffer.data(), HEADER_SIZE + size)))
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ": failed." << std::endl;
  return true;
}

} // end of namespace fifo
