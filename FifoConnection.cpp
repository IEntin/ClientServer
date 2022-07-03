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

namespace fifo {

FifoConnection::FifoConnection(const ServerOptions& options,
			       TaskControllerPtr taskController,
			       std::string_view fifoName,
			       RunnablePtr parent) :
  Runnable(&(parent->_stopped), &(taskController->_totalConnections), &(parent->_typedConnections)),
  _options(options),
  _taskController(taskController),
  _fifoName(fifoName),
  // save for reference count
  _parent(parent) {
  // - TaskController - FifoServer - TcpServer - waiting TcpConnection
  int total = _totalConnections - 4;
  // - FifoServer
  int fifoConnections = _typedConnections - 1;
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
       << ":total connections=" << total << ",fifo connections="
       << fifoConnections << '\n';
}

FifoConnection::~FifoConnection() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '\n';
}

void FifoConnection::run() {
  while (!_stopped) {
    _response.clear();
    HEADER header;
    _uncompressedRequest.clear();
    if (!receiveRequest(_uncompressedRequest, header))
      continue;
    _taskController->submitTask(header, _uncompressedRequest, _response);
    sendResponse(_response);
  }
}

bool FifoConnection::receiveRequest(std::vector<char>& message, HEADER& header) {
  utility::CloseFileDescriptor cfdr(_fdRead);
  _fdRead = open(_fifoName.data(), O_RDONLY);
  if (_fdRead == -1) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-' 
	 << std::strerror(errno) << ' ' << _fifoName << '\n';
    return false;
  }
  header = Fifo::readHeader(_fdRead, _options._numberRepeatEINTR);
  const auto& [uncomprSize, comprSize, compressor, diagnostics, headerDone] = header;
  if (!headerDone) {
    if (_options._destroyBufferOnClientDisconnect)
      MemoryPool::destroyBuffers();
    return false;
  }
  return readMsgBody(_fdRead, uncomprSize, comprSize, compressor == COMPRESSORS::LZ4, message);
}

bool FifoConnection::readMsgBody(int fd,
				 size_t uncomprSize,
				 size_t comprSize,
				 bool bcompressed,
				 std::vector<char>& uncompressed) {
  static auto& printOnce[[maybe_unused]] =
    CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << (bcompressed ? " received compressed" : " received not compressed") << '\n';
  if (bcompressed) {
    std::vector<char>& buffer = MemoryPool::instance().getFirstBuffer(comprSize);
    if (!Fifo::readString(fd, buffer.data(), comprSize, _options._numberRepeatEINTR))
      return false;
    std::string_view received(buffer.data(), comprSize);
    uncompressed.resize(uncomprSize);
    if (!Compression::uncompress(received, uncompressed)) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	   << ":failed to uncompress payload.\n";
      return false;
    }
  }
  else {
    uncompressed.resize(uncomprSize);
    if (!Fifo::readString(fd, uncompressed.data(), uncomprSize, _options._numberRepeatEINTR))
      return false;
  }
  return true;
}

bool FifoConnection::sendResponse(const Response& response) {
  std::string_view message =
    serverutility::buildReply(response, _options._compressor);
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
