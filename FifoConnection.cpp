/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoConnection.h"
#include "Compression.h"
#include "Fifo.h"
#include "Header.h"
#include "MemoryPool.h"
#include "ServerOptions.h"
#include "ServerUtility.h"
#include "TaskController.h"
#include "Utility.h"
#include <cassert>
#include <cstring>
#include <fcntl.h>
#include <filesystem>

namespace fifo {

FifoConnection::FifoConnection(const ServerOptions& options,
			       std::string_view fifoName,
			       RunnablePtr parent) :
  Runnable(parent, TaskController::instance(), parent, "fifo", options._maxFifoConnections),
  _options(options), _fifoName(fifoName), _parent(parent) {}

FifoConnection::~FifoConnection() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '\n';
}

void FifoConnection::run() {
  setRunning();
  while (!_stopped) {
    _response.clear();
    HEADER header;
    _uncompressedRequest.clear();
    if (!receiveRequest(_uncompressedRequest, header))
      break;;
    TaskController::instance()->submitTask(header, _uncompressedRequest, _response);
    if (!sendResponse(_response))
      break;
  }
  std::filesystem::remove(_fifoName);
}

bool FifoConnection::start() {
  return true;
}

void FifoConnection::stop() {
  wakeupPipe();
}

void FifoConnection::wakeupPipe() {
  Fifo::onExit(_fifoName, _options._numberRepeatENXIO, _options._ENXIOwait);
}

bool FifoConnection::receiveRequest(std::vector<char>& message, HEADER& header) {
  utility::CloseFileDescriptor cfdr(_fdRead);
  _fdRead = open(_fifoName.data(), O_RDONLY);
  if (_fdRead == -1) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-' 
	 << std::strerror(errno) << ' ' << _fifoName << '\n';
    return false;
  }
  return serverutility::receiveRequest(_fdRead, message, header, _options);
}

bool FifoConnection::sendResponse(const Response& response) {
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
      std::this_thread::sleep_for(std::chrono::microseconds(_options._ENXIOwait));
  } while (_fdWrite == -1 &&
	   (errno == ENXIO || errno == EINTR) && rep++ < _options._numberRepeatENXIO);
  if (_fdWrite == -1) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	 << std::strerror(errno) << ' ' << _fifoName << '\n';
    if (_options._destroyBufferOnClientDisconnect)
      MemoryPool::destroyBuffers();
    return false;
  }
  if (_options._setPipeSize)
    Fifo::setPipeSize(_fdWrite, message.size());
  return Fifo::writeString(_fdWrite, message);
}

} // end of namespace fifo
