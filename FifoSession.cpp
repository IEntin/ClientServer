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
#include <cassert>
#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>
#include <sys/types.h>

namespace fifo {

std::atomic<unsigned> FifoSession::_numberObjects;

FifoSession::FifoSession(const ServerOptions& options, unsigned short ephemeralIndex, RunnablePtr parent) :
  Runnable(parent, TaskController::instance(), parent, FIFO, options._maxFifoSessions),
  _options(options), _parent(parent), _ephemeralIndex(ephemeralIndex) {
  _numberObjects++;
  char array[5] = {};
  std::string_view baseName = utility::toStringView(_ephemeralIndex, array, sizeof(array)); 
  _fifoName.append(_options._fifoDirectoryName).append(1,'/').append(baseName.data(), baseName.size());
}

FifoSession::~FifoSession() {
  _numberObjects--;
  std::filesystem::remove(_fifoName);
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

void FifoSession::run() {
  if (!std::filesystem::exists(_fifoName))
    // waiting client was closed
    return;
  while (!_stopped) {
    _response.clear();
    HEADER header;
    _uncompressedRequest.clear();
    if (!receiveRequest(_uncompressedRequest, header))
      break;
    TaskController::instance()->submitTask(header, _uncompressedRequest, _response);
    if (!sendResponse(_response))
      break;
  }
}

unsigned FifoSession::getNumberObjects() const {
  return _numberObjects;
}

bool FifoSession::start() {
  if (mkfifo(_fifoName.data(), 0620) == -1 && errno != EEXIST) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	 << std::strerror(errno) << '-' << _fifoName << '\n';
    return false;
  }
  return true;
}

void FifoSession::stop() {
  Fifo::onExit(_fifoName, _options._numberRepeatENXIO, _options._ENXIOwait);
}

bool FifoSession::receiveRequest(std::vector<char>& message, HEADER& header) {
  utility::CloseFileDescriptor cfdr(_fdRead);
  _fdRead = open(_fifoName.data(), O_RDONLY);
  if (_fdRead == -1) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-' 
	 << std::strerror(errno) << ' ' << _fifoName << '\n';
    return false;
  }
  return serverutility::receiveRequest(_fdRead, message, header, _options);
}

bool FifoSession::sendResponse(const Response& response) {
  std::string_view message =
    serverutility::buildReply(response, _options._compressor, 0);
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
	 << std::strerror(errno) << ' ' << _fifoName << '\n';
    MemoryPool::destroyBuffers();
    return false;
  }
  if (_options._setPipeSize)
    Fifo::setPipeSize(_fdWrite, message.size());
  return Fifo::writeString(_fdWrite, message);
}

} // end of namespace fifo
