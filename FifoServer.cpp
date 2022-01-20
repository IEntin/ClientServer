/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoServer.h"
#include "Fifo.h"
#include "ProgramOptions.h"
#include "Task.h"
#include "Utility.h"
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <sys/stat.h>

extern volatile std::atomic<bool> stopFlag;

namespace fifo {

std::vector<FifoServer> FifoServer::_runnables;
std::vector<std::thread> FifoServer::_threads;
const std::string FifoServer::_fifoDirectoryName = ProgramOptions::get("FifoDirectoryName", std::string());

FifoServer::FifoServer(const std::string& fifoName) :
  _fifoName(fifoName) {}

FifoServer::~FifoServer() {
  close(_fdRead);
  close(_fdWrite);
}

// start threads - one for each client
bool FifoServer::startThreads() {
  // in case there was no proper shudown.
  removeFifoFiles();
  std::string fifoBaseNamesStr = ProgramOptions::get("FifoBaseNames", std::string());
  std::vector<std::string> fifoBaseNameVector;
  utility::split(fifoBaseNamesStr, fifoBaseNameVector, ",\n ");
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
    _runnables.emplace_back(fifoName);
    _threads.emplace_back(_runnables.back());
  }
  return true;
}

void FifoServer::joinThreads() {
  for (auto& runnable : _runnables) {
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
    close(fd);
  }
  for (auto& thread : _threads)
    if (thread.joinable())
      thread.join();
  removeFifoFiles();
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	    << " ... fifoThreads joined ..." << std::endl;
  // silence valgrind.
  std::vector<std::thread>().swap(_threads);
  std::vector<FifoServer>().swap(FifoServer::_runnables);
}

template <typename C>
bool FifoServer::receiveRequest(C& batch) {
  close(_fdWrite);
  _fdWrite = -1;
  if (!stopFlag) {
    _fdRead = open(_fifoName.c_str(), O_RDONLY);
    if (_fdRead == -1) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< '-' << std::strerror(errno) << std::endl;
      return false;
    }
  }
  return Fifo::receive(_fdRead, batch) && !batch.empty();
}

bool FifoServer::sendResponse(Batch& response) {
  close(_fdRead);
  _fdRead = -1;
  if (!stopFlag) {
    _fdWrite = open(_fifoName.c_str(), O_WRONLY);
    if (_fdWrite == -1) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< '-' << std::strerror(errno) << std::endl;
      return false;
    }
  }
  return Fifo::sendReply(_fdWrite, response);
}

void FifoServer::operator()() noexcept {
  static const bool useStringView = ProgramOptions::get("StringTypeInTask", std::string()) == "STRINGVIEW";
  while (!stopFlag) {
    _response.clear();
    if (useStringView) {
      _uncompressedRequest.clear();
      if (!receiveRequest(_uncompressedRequest))
	break;
      TaskSV::process(_uncompressedRequest, _response);
    }
    else {
      _requestBatch.clear();
      if (!receiveRequest(_requestBatch))
	break;
      TaskST::process(_requestBatch, _response);
    }
    if (!sendResponse(_response))
      break;
  }
}

void FifoServer::removeFifoFiles() {
  for(auto const& entry : std::filesystem::directory_iterator(_fifoDirectoryName))
    if (entry.is_fifo())
      std::filesystem::remove(entry);
}

} // end of namespace fifo
