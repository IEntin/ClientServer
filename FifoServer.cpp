/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoServer.h"
#include "Fifo.h"
#include "Task.h"
#include "Utility.h"
#include <fcntl.h>
#include <filesystem>
#include <cstring>
#include <iostream>
#include <sys/stat.h>

namespace {

volatile std::atomic<bool> stopFlag;

} // end of anonimous namespace

namespace fifo {

std::string FifoServer::_fifoDirectoryName;
std::vector<FifoServer> FifoServer::_fifoThreads;

FifoServer::FifoServer(const std::string& fifoName) :
  _runnable(fifoName), _thread(_runnable) {
}

FifoServer::FifoServer(FifoServer&& other) :
  _runnable(std::move(other._runnable)), _thread(std::move(other._thread)) {}

// start threads - one for each client

bool FifoServer::startThreads(const std::string& fifoDirName, const std::string& fifoBaseNames) {
  _fifoDirectoryName = fifoDirName;
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
    _fifoThreads.emplace_back(fifoName);
  }
  return true;
}

FifoServer::Runnable::Runnable(const std::string& fifoName) :
  _fifoName(fifoName) {}

FifoServer::Runnable::~Runnable() {
  if (_fdRead != -1)
    close(_fdRead);
  if (_fdWrite != -1)
    close(_fdWrite);
}

void FifoServer::Runnable::operator()() noexcept {
  while (!stopFlag) {
    _response.clear();
    HEADER header;
    _uncompressedRequest.clear();
    if (!receiveRequest(_uncompressedRequest, header))
      break;
    Task::process(header, _uncompressedRequest, _response);
    if (!sendResponse(_response))
      continue;
  }
}

bool FifoServer::Runnable::receiveRequest(std::vector<char>& batch, HEADER& header) {
  if (_fdWrite != -1) {
    close(_fdWrite);
    _fdWrite = -1;
  }
  if (!stopFlag) {
    _fdRead = open(_fifoName.c_str(), O_RDONLY);
    if (_fdRead == -1) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< '-' << std::strerror(errno) << std::endl;
      return false;
    }
  }
  return Fifo::receive(_fdRead, batch, header) && !batch.empty();
}

bool FifoServer::Runnable::sendResponse(Batch& response) {
  if (_fdRead != -1) {
    close(_fdRead);
    _fdRead = -1;
  }
  if (!stopFlag) {
    _fdWrite = open(_fifoName.c_str(), O_WRONLY | O_NONBLOCK);
    if (_fdWrite == -1) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< '-' << std::strerror(errno) << std::endl;
      return false;
    }
  }
  return Fifo::sendReply(_fdWrite, response);
}

void FifoServer::removeFifoFiles() {
  for(auto const& entry : std::filesystem::directory_iterator(_fifoDirectoryName))
    if (entry.is_fifo())
      std::filesystem::remove(entry);
}

void FifoServer::joinThreads() {
  stopFlag.store(true);
  for (auto& [runnable, thread] : _fifoThreads) {
    int fd = open(runnable._fifoName.c_str(), O_WRONLY);
    if (fd == -1) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
		<< std::strerror(errno) << '-' << runnable._fifoName << std::endl;
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
  for (auto& [runnable, thread] : _fifoThreads)
    if (thread.joinable())
      thread.join();
  removeFifoFiles();
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	    << " ... fifoThreads joined ..." << std::endl;
  std::vector<FifoServer>().swap(FifoServer::_fifoThreads);
}

} // end of namespace fifo
