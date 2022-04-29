/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoServer.h"
#include "Compression.h"
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
#include <poll.h>
#include <sys/stat.h>

namespace fifo {

FifoServer::FifoServer(TaskControllerPtr taskController,
		       std::string_view fifoDirName,
		       std::string_view fifoBaseNames,
		       COMPRESSORS compressor) :
  _taskController(taskController), _fifoDirName(fifoDirName), _compressor(compressor) {
  // in case there was no proper shudown.
  removeFifoFiles();
  std::vector<std::string> fifoBaseNameVector;
  utility::split(fifoBaseNames, fifoBaseNameVector, ",\n ");
  if (fifoBaseNameVector.empty()) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << "-empty fifo base names vector" << std::endl;
    return;
  }
  for (const auto& baseName : fifoBaseNameVector)
    _fifoNames.emplace_back(_fifoDirName + '/' + baseName);
}

FifoServer::~FifoServer() {
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool FifoServer::start() {
  for (const auto& fifoName : _fifoNames) {
    if (mkfifo(fifoName.data(), 0620) == -1 && errno != EEXIST) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
		<< std::strerror(errno) << '-' << fifoName << std::endl;
      return false;
    }
    FifoConnectionPtr connection =
      std::make_shared<FifoConnection>(_taskController, fifoName, _compressor, shared_from_this());
    pushToThreadPool(connection);
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
	std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << " result="
		  << result << ":expected result == 1 " << std::strerror(errno) << std::endl;
      close(fd);
    }
    else {
      fd = open(fifoName.data(), O_RDONLY | O_NONBLOCK);
      if (fd == -1) {
	std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
		  << std::strerror(errno) << '-' << fifoName << std::endl;
	continue;
      }
      char c{};
      int result[[maybe_unused]] = read(fd, &c, 1);
      close(fd);
    }
  }
}

void FifoServer::pushToThreadPool(FifoConnectionPtr connection) {
  _threadPool.push(connection);
}

// class FifoConnection

FifoConnection::FifoConnection(TaskControllerPtr taskController,
			       std::string_view fifoName,
			       COMPRESSORS compressor,
			       FifoServerPtr server) :
  _taskController(taskController), _fifoName(fifoName), _compressor(compressor), _server(server) {}

FifoConnection::~FifoConnection() {
  if (_fdRead != -1)
    close(_fdRead);
  if (_fdWrite != -1)
    close(_fdWrite);
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
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
      if (!sendResponse(_response))
	std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
		  << std::strerror(errno) << '-' << _fifoName << std::endl;
    }
    catch (...) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< " ! exception caught " << _fifoName << std::endl;
      break;
    }
  }
}

bool FifoConnection::receiveRequest(std::vector<char>& message, HEADER& header) {
  if (_fdWrite != -1) {
    close(_fdWrite);
    _fdWrite = -1;
  }
  if (!_server->stopped()) {
    _fdRead = open(_fifoName.data(), O_RDONLY);
    if (_fdRead == -1) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-' 
		<< std::strerror(errno) << ' ' << _fifoName << std::endl;
      return false;
    }
  }
  //Fifo::pollFd(_fdRead, POLLIN, _fifoName);
  header = Fifo::readHeader(_fdRead);
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
  if (bcompressed) {
    static auto& printOnce[[maybe_unused]] = std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
						       << " received compressed" << std::endl;
    std::vector<char>& buffer = _taskController->getMemoryPool().getPrimaryBuffer(comprSize);
    if (!Fifo::readString(fd, buffer.data(), comprSize))
      return false;
    std::string_view received(buffer.data(), comprSize);
    uncompressed.resize(uncomprSize);
    if (!Compression::uncompress(received, uncompressed)) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ":failed to uncompress payload" << std::endl;
      return false;
    }
  }
  else {
    static auto& printOnce[[maybe_unused]] = std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
						       << " received not compressed" << std::endl;
    uncompressed.resize(uncomprSize);
    if (!Fifo::readString(fd, uncompressed.data(), uncomprSize))
      return false;
  }
  return true;
}

bool FifoConnection::sendResponse(Batch& response) {
  if (_fdRead != -1) {
    close(_fdRead);
    _fdRead = -1;
  }
  if (!_server->stopped()) {
    _fdWrite = open(_fifoName.data(), O_WRONLY);
    if (_fdWrite == -1) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
		<< std::strerror(errno) << ' ' << _fifoName << std::endl;
      return false;
    }
  }
  //Fifo::pollFd(_fdWrite, POLLOUT, _fifoName);
  std::string_view message =
    serverutility::buildReply(response,_compressor, _taskController->getMemoryPool());
  if (message.empty())
    return false;
  return Fifo::writeString(_fdWrite, message);
}

} // end of namespace fifo
