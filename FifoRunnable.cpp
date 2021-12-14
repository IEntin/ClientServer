#include "FifoRunnable.h"
#include "Fifo.h"
#include "ProgramOptions.h"
#include "TaskTemplate.h"
#include "Utility.h"
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>

namespace fifo {

std::vector<FifoRunnable> FifoRunnable::_runnables;
std::vector<std::thread> FifoRunnable::_threads;
const std::string FifoRunnable::_fifoDirectoryName = ProgramOptions::get("FifoDirectoryName", std::string());
volatile std::atomic<bool> FifoRunnable::_stopFlag(false);
std::mutex FifoRunnable::_stopMutex;
std::condition_variable FifoRunnable::_stopCondition;

FifoRunnable::FifoRunnable(const std::string& fifoName) :
  _fifoName(fifoName) {}

FifoRunnable::~FifoRunnable() {
  close(_fdRead);
  close(_fdWrite);
}

// start threads - one for each client
bool FifoRunnable::startThreads() {
  const std::string emptyString;
  std::string fifoBaseNamesStr = ProgramOptions::get("FifoBaseNames", emptyString);
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

void FifoRunnable::stop() {
  std::lock_guard lock(_stopMutex);
  _stopFlag.store(true);
  _stopCondition.notify_one();
}

void FifoRunnable::joinThreads() {
  std::unique_lock lock(_stopMutex);
  _stopCondition.wait(lock, []{ return _stopFlag.load(); });
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
  for(auto const& entry : std::filesystem::directory_iterator(_fifoDirectoryName))
    if (entry.is_fifo())
      std::filesystem::remove(entry);
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	    << " ... fifoThreads joined ..." << std::endl;
  // silence valgrind false positives.
  std::vector<std::thread>().swap(_threads);
  std::vector<FifoRunnable>().swap(FifoRunnable::_runnables);
}

bool FifoRunnable::receiveRequest(Batch& batch) {
  close(_fdWrite);
  _fdWrite = -1;
  if (!_stopFlag) {
    _fdRead = open(_fifoName.c_str(), O_RDONLY);
    if (_fdRead == -1) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< '-' << std::strerror(errno) << std::endl;
      return false;
    }
  }
  return Fifo::receive(_fdRead, batch) && !batch.empty();
}

bool FifoRunnable::receiveRequest(std::vector<char>& uncompressed) {
  close(_fdWrite);
  _fdWrite = -1;
  if (!_stopFlag) {
    _fdRead = open(_fifoName.c_str(), O_RDONLY);
    if (_fdRead == -1) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< '-' << std::strerror(errno) << std::endl;
      return false;
    }
  }
  return Fifo::receive(_fdRead, uncompressed) && !uncompressed.empty();
}

bool FifoRunnable::sendResponse(Batch& response) {
  close(_fdRead);
  _fdRead = -1;
  if (!_stopFlag) {
    _fdWrite = open(_fifoName.c_str(), O_WRONLY);
    if (_fdWrite == -1) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< '-' << std::strerror(errno) << std::endl;
      return false;
    }
  }
  return Fifo::sendReply(_fdWrite, response);
}

void FifoRunnable::operator()() noexcept {
  static const bool useStringView = ProgramOptions::get("StringTypeInTask", std::string()) == "STRINGVIEW";
    if (useStringView) {
      while (!_stopFlag) {
	// We cannot use MemoryPool because vector 'uncompressed' will be swapped with 'Task::_rawInput'.
	// Instead we make this vector as well as 'Task::_rawInput' static thread_local.
	// Then we allocate only few times and later swap with the vector of close and eventually
	// the same maximum required capacity. No more allocations until input pattern changes.
	// We need to clear this vector on every iteration which does not change its capacity.
	static thread_local std::vector<char> uncompressed;
	uncompressed.clear();
	if (!receiveRequest(uncompressed))
	  break;
	Batch response;
	TaskSV::process(_fifoName, uncompressed, response);
	if (!sendResponse(response))
	  break;
      }
    }
    else {
      while (!_stopFlag) {
	Batch batch;
	if (!receiveRequest(batch))
	  break;
	Batch response;
	TaskST::process(_fifoName, batch, response);
	if (!sendResponse(response))
	  break;
      }
    }
}

} // end of namespace fifo
