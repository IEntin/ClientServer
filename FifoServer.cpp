/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoServer.h"
#include "Compression.h"
#include "ServerOptions.h"
#include "ServerUtility.h"
#include "Fifo.h"
#include "Header.h"
#include "MemoryPool.h"
#include "TaskController.h"
#include "Utility.h"
#include <cassert>
#include <cstring>
#include <filesystem>
#include <fcntl.h>
#include <sys/stat.h>

namespace fifo {

FifoServer::FifoServer(TaskControllerPtr taskController, const ServerOptions& options) :
  _taskController(taskController),
  _fifoDirName(options._fifoDirectoryName),
  _compressor(options._compressor) {
  // in case there was no proper shudown.
  removeFifoFiles();
  std::vector<std::string> fifoBaseNameVector;
  utility::split(options._fifoBaseNames, fifoBaseNameVector, ",\n ");
  if (fifoBaseNameVector.empty()) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << "-empty fifo base names vector" << std::endl;
    return;
  }
  for (const auto& baseName : fifoBaseNameVector)
    _fifoNames.emplace_back(_fifoDirName + '/' + baseName);
}

FifoServer::~FifoServer() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool FifoServer::start(const ServerOptions& options) {
  for (const auto& fifoName : _fifoNames) {
    if (mkfifo(fifoName.data(), 0620) == -1 && errno != EEXIST) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
		<< std::strerror(errno) << '-' << fifoName << std::endl;
      return false;
    }
    FifoConnectionPtr connection =
      std::make_shared<FifoConnection>(options, _taskController, fifoName, _compressor, shared_from_this());
    _threadPool.push(connection);
  }
  _threadPool.start(_fifoNames.size());
  return true;
}

void FifoServer::stop() {
  _stopped.store(true);
  wakeupPipes();
  _threadPool.stop();
  removeFifoFiles();
}

void FifoServer::removeFifoFiles() {
  for(auto const& entry : std::filesystem::directory_iterator(_fifoDirName))
    if (entry.is_fifo())
      std::filesystem::remove(entry);
}

void FifoServer::wakeupPipes() {
  for (const auto& fifoName : _fifoNames) {
    int fd = open(fifoName.data(), O_WRONLY | O_NONBLOCK);
    if (fd != -1) {
      char c = 's';
      int result = write(fd, &c, 1);
      if (result != 1)
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << " result="
		  << result << ":expected result == 1 " << std::strerror(errno) << std::endl;
      close(fd);
    }
  }
}

// class FifoConnection

FifoConnection::FifoConnection(const ServerOptions& options,
			       TaskControllerPtr taskController,
			       std::string_view fifoName,
			       COMPRESSORS compressor,
			       FifoServerPtr server) :
  _options(options), _taskController(taskController), _fifoName(fifoName), _compressor(compressor), _server(server) {}

FifoConnection::~FifoConnection() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

void FifoConnection::run() noexcept {
  while (!_server->stopped()) {
    try {
      _response.clear();
      HEADER header;
      _uncompressedRequest.clear();
      if (!receiveRequest(_uncompressedRequest, header))
	continue;
      _taskController->submitTask(header, _uncompressedRequest, _response);
      sendResponse(_response);
    }
    catch (...) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< " ! exception caught " << _fifoName << std::endl;
      break;
    }
  }
}

bool FifoConnection::receiveRequest(std::vector<char>& message, HEADER& header) {
  utility::CloseFileDescriptor cfdr(_fdRead);
  if (!_server->stopped()) {
    _fdRead = open(_fifoName.data(), O_RDONLY);
    if (_fdRead == -1) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-' 
		<< std::strerror(errno) << ' ' << _fifoName << std::endl;
      return false;
    }
  }
  header = Fifo::readHeader(_fdRead, _options._numberRepeatEINTR);
  const auto& [uncomprSize, comprSize, compressor, diagnostics, headerDone] = header;
  if (!headerDone)
    return false;
  return readMsgBody(_fdRead, uncomprSize, comprSize, compressor == COMPRESSORS::LZ4, message);
}

bool FifoConnection::readMsgBody(int fd,
				 size_t uncomprSize,
				 size_t comprSize,
				 bool bcompressed,
				 std::vector<char>& uncompressed) {
  static auto& printOnce[[maybe_unused]] =
    CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
      << (bcompressed ? " received compressed" : " received not compressed") << std::endl;
  if (bcompressed) {
    std::vector<char>& buffer = _taskController->getMemoryPool().getBuffer(comprSize);
    if (!Fifo::readString(fd, buffer.data(), comprSize, _options._numberRepeatEINTR))
      return false;
    std::string_view received(buffer.data(), comprSize);
    uncompressed.resize(uncomprSize);
    if (!Compression::uncompress(received, uncompressed)) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ":failed to uncompress payload" << std::endl;
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
    serverutility::buildReply(response, _compressor, _taskController->getMemoryPool());
  if (message.empty())
    return false;
  // Open write fd in NONBLOCK mode in order to protect the server
  // from a client crashed or killed with SIGKILL and resume operation
  // after a client restarted (in block mode the server will just hang on
  // open(...) no matter what).
  utility::CloseFileDescriptor cfdw(_fdWrite);
  if (!_server->stopped()) {
    int rep = 0;
    do {
      _fdWrite = open(_fifoName.data(), O_WRONLY | O_NONBLOCK);
      if (_fdWrite == -1 && (errno == ENXIO || errno == EINTR))
	std::this_thread::sleep_for(std::chrono::microseconds(_options._ENXIOwait));
    } while (_fdWrite == -1 && !_server->stopped() &&
	     (errno == ENXIO || errno == EINTR) && rep++ < _options._numberRepeatENXIO);
  }
  if (_server->stopped())
    return false;
  if (_fdWrite == -1) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	      << std::strerror(errno) << ' ' << _fifoName << std::endl;
    MemoryPool::destroyBuffer();
    return false;
  }
  if (_options._setPipeSize)
    Fifo::setPipeSize(_fdWrite, message.size());
  return Fifo::writeString(_fdWrite, message);
}

} // end of namespace fifo
