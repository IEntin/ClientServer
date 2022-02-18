/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoServer.h"
#include "ServerUtility.h"
#include "Compression.h"
#include "Fifo.h"
#include "MemoryPool.h"
#include "Task.h"
#include "Utility.h"
#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <iostream>

namespace {

volatile std::atomic<bool> stopFlag;

} // end of anonimous namespace

namespace fifo {

std::string FifoServer::_fifoDirectoryName;
std::pair<COMPRESSORS, bool> FifoServer::_compression;
std::vector<FifoServerPtr> FifoServer::_fifoThreads;

FifoServer::FifoServer(const std::string& fifoName) : _fifoName(fifoName) {}

FifoServer::~FifoServer() {
  if (_fdRead != -1)
    close(_fdRead);
  if (_fdWrite != -1)
    close(_fdWrite);
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

void FifoServer::start() {
  _thread = std::thread(&FifoServer::run, shared_from_this());
}

// start threads - one for each client

bool FifoServer::startThreads(const std::string& fifoDirName,
			      const std::string& fifoBaseNames,
			      const std::pair<COMPRESSORS, bool>& compression) {
  _fifoDirectoryName = fifoDirName;
  _compression = compression;
  // in case there was no proper shudown.
  removeFifoFiles();
  std::vector<std::string> fifoBaseNameVector;
  utility::split(fifoBaseNames, fifoBaseNameVector, ",\n ");
  if (fifoBaseNameVector.empty()) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << "-empty fifo base names vector" << std::endl;
    return false;
  }
  for (size_t i = 0; i < fifoBaseNameVector.size(); ++i) {
    std::string fifoName = _fifoDirectoryName + '/' + fifoBaseNameVector[i];
    if (mkfifo(fifoName.c_str(), 0620) == -1 && errno != EEXIST) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
		<< std::strerror(errno) << '-' << fifoName << std::endl;
      return false;
    }
    FifoServerPtr server = std::make_shared<FifoServer>(fifoName);
    server->start();
    _fifoThreads.emplace_back(server);
  }
  return true;
}

void FifoServer::run() noexcept {
  while (!stopFlag) {
    try {
      _response.clear();
      HEADER header;
      _uncompressedRequest.clear();
      if (!receiveRequest(_uncompressedRequest, header))
	continue;
      Task::process(header, _uncompressedRequest, _response);
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

bool FifoServer::receiveRequest(std::vector<char>& message, HEADER& header) {
  if (_fdWrite != -1) {
    close(_fdWrite);
    _fdWrite = -1;
  }
  if (!stopFlag) {
    _fdRead = open(_fifoName.c_str(), O_RDONLY);
    if (_fdRead == -1) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-' 
		<< std::strerror(errno) << ' ' << _fifoName << std::endl;
      return false;
    }
  }
  header = Fifo::readHeader(_fdRead);
  const auto& [uncomprSize, comprSize, compressor, diagnostics, headerDone] = header;
  if (!headerDone)
    return false;
  return readMsgBody(_fdRead, uncomprSize, comprSize, compressor == COMPRESSORS::LZ4, message);
}

bool FifoServer::readMsgBody(int fd,
			     size_t uncomprSize,
			     size_t comprSize,
			     bool bcompressed,
			     std::vector<char>& uncompressed) {
  if (bcompressed) {
    static auto& printOnce[[maybe_unused]] = std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
						       << " received compressed" << std::endl;
    auto[buffer, bufferSize] = MemoryPool::getPrimaryBuffer(comprSize);
    if (!Fifo::readString(fd, buffer, comprSize))
      return false;
    std::string_view received(buffer, comprSize);
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

bool FifoServer::sendResponse(Batch& response) {
  if (_fdRead != -1) {
    close(_fdRead);
    _fdRead = -1;
  }
  if (!stopFlag) {
    _fdWrite = open(_fifoName.c_str(), O_WRONLY | O_NONBLOCK);
    if (_fdWrite == -1) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
		<< std::strerror(errno) << ' ' << _fifoName << std::endl;
      return false;
    }
    do {
      pollfd pfd{ _fdWrite, POLLOUT, -1 };
      pfd.revents = 0;
      int presult = poll(&pfd, 1, -1);
      if (presult <= 0) {
	if (errno == EINTR)
	  continue;
	std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
		  << std::strerror(errno) << ' ' << _fifoName << std::endl;
	close(_fdWrite);
	_fdWrite = -1;
	return false;
      }
      assert(pfd.revents & POLLOUT);
    } while (errno == EINTR);
  }
  std::string_view message = serverutility::buildReply(response, _compression);
  if (message.empty())
    return false;
  return Fifo::writeString(_fdWrite, message);
}

void FifoServer::removeFifoFiles() {
  for(auto const& entry : std::filesystem::directory_iterator(_fifoDirectoryName))
    if (entry.is_fifo())
      std::filesystem::remove(entry);
}

void FifoServer::joinThreads() {
  stopFlag.store(true);
  for (auto server : _fifoThreads) {
    int fd = open(server->_fifoName.c_str(), O_WRONLY);
    if (fd == -1) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
		<< std::strerror(errno) << '-' << server->_fifoName << std::endl;
      continue;
    }
    char c = 's';
    int result = write(fd, &c, 1);
    if (result != 1)
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << " result="
		<< result << ":expected result == 1 " << std::strerror(errno) << std::endl;
    if (fd != -1)
      close(fd);
  }
  for (auto server : _fifoThreads)
    if (server->_thread.joinable())
      server->_thread.join();
  removeFifoFiles();
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	    << " ... fifoThreads joined ..." << std::endl;
  std::vector<FifoServerPtr>().swap(FifoServer::_fifoThreads);
  stopFlag.store(false);
}

} // end of namespace fifo
