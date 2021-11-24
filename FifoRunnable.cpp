#include "FifoRunnable.h"
#include "Fifo.h"
#include "ProgramOptions.h"
#include "TaskTemplate.h"
#include "Utility.h"
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <poll.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace fifo {

std::vector<FifoRunnable> FifoRunnable::_runnables;
std::vector<std::thread> FifoRunnable::_threads;
const std::string FifoRunnable::_fifoDirectoryName = ProgramOptions::get("FifoDirectoryName", std::string());
volatile std::atomic<bool> FifoRunnable::_stopFlag(false);
std::mutex FifoRunnable::_stopMutex;
std::condition_variable FifoRunnable::_stopCondition;

FifoRunnable::FifoRunnable(const std::string& receiveFifoName, std::string sendFifoName) :
  _receiveFifoName(receiveFifoName), _sendFifoName(sendFifoName) {}

// start threads - one for each client
bool FifoRunnable::startThreads() {
  const std::string emptyString;
  std::string receiveIdStr = ProgramOptions::get("ReceiveIds", emptyString);
  std::vector<std::string> receiveIdVector;
  utility::split(receiveIdStr, receiveIdVector, ",\n ");
  if (receiveIdVector.empty()) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << "-empty reveive id vector" << std::endl;
    return false;
  }
  std::string sendIdStr = ProgramOptions::get("SendIds", emptyString);
  std::vector<std::string> sendIdVector;
  utility::split(sendIdStr, sendIdVector, ",\n ");
  assert(receiveIdVector.size() == sendIdVector.size());
  for (size_t i = 0; i < receiveIdVector.size(); ++i) {
    std::string receiveFifoName = _fifoDirectoryName + '/' + receiveIdVector[i];
    std::string sendFifoName = _fifoDirectoryName + '/' + sendIdVector[i];
    if (mkfifo(receiveFifoName.c_str(), 0620) == -1 && errno != EEXIST) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
		<< std::strerror(errno) << '-' << receiveFifoName << std::endl;
      return false;
    }
    if (mkfifo(sendFifoName.c_str(), 0640) == -1 && errno != EEXIST) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
		<< std::strerror(errno) << '-' << sendFifoName << std::endl;
      return false;
    }
    _runnables.emplace_back(FifoRunnable(receiveFifoName, sendFifoName));
    _threads.emplace_back(FifoRunnable(receiveFifoName, sendFifoName));
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
    int fd = open(runnable._receiveFifoName.c_str(), O_WRONLY);
    if (fd == -1) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< '-' << std::strerror(errno) << '-'
		<< runnable._receiveFifoName << std::endl;
      continue;
    }
    char c = 's';
    int result = write(fd, &c, 1);
    if (result != 1)
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< "-expected result == 1" << std::endl;
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

bool FifoRunnable::waitRequest() {
  unsigned rep = 0;
  do {
    errno = 0;
    pollfd pfd{ _fdRead, POLLIN, -1 };
    int presult = poll(&pfd, 1, -1);
    if (presult <= 0) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< '-' << std::strerror(errno) << std::endl;
      if (errno == EINTR)
	continue;
      return false;
    }
    else if (pfd.revents & POLLERR) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< '-' << std::strerror(errno) << std::endl;
      return false;
    }
    else if (pfd.revents & POLLHUP) {
      if (!_stopFlag)
	std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		  << ":POLLHUP detected " << _receiveFifoName << std::endl;
      close(_fdRead);
      _fdRead = -1;
      close(_fdWrite);
      _fdWrite = -1;
      return false;
    }
  } while (errno == EINTR && rep++ < 3 && !_stopFlag);
  return !_stopFlag;
}

bool FifoRunnable::receiveRequest(Batch& batch) {
  return Fifo::receive(_fdRead, batch) && !batch.empty();
}

bool FifoRunnable::receiveRequest(std::vector<char>& received) {
  return Fifo::receive(_fdRead, received) && !received.empty();
}

bool FifoRunnable::sendResponse(Batch& response) {
  if (_fdWrite == -1) {
    _fdWrite = open(_sendFifoName.c_str(), O_WRONLY);
    if (_fdWrite == -1) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
		<< std::strerror(errno) << '-' << _sendFifoName << std::endl;
      return false;
    }
  }
  return Fifo::sendReply(_fdWrite, response);
}

bool FifoRunnable::reopenFD() {
  if (_fdRead != -1)
    close(_fdRead);
  _fdRead = open(_receiveFifoName.c_str(), O_RDONLY);
  if (_fdRead == -1) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << '-' << std::strerror(errno) << std::endl;
    return false;
  }
  if (_fdWrite != -1)
    close(_fdWrite);
  _fdWrite = -1;
  return true;
}

void FifoRunnable::operator()() noexcept {
  static const bool useStringView = ProgramOptions::get("StringTypeInTask", std::string()) == "STRINGVIEW";
  while (!_stopFlag) {
    if (!reopenFD())
      return;
    if (useStringView) {
      while (!_stopFlag) {
	if (!waitRequest())
	  break;
	// We cannot use MemoryPool because vector 'received' will be swapped with 'Task::_rawInput'.
	// Instead we make this vector as well as 'Task::_rawInput' static thread_local.
	// Then we allocate only few times and later swap with the vector of close and eventually
	// the same maximum required capacity. No more allocations until input pattern changes.
	// We need to clear this vector on every iteration which does not change its capacity.
	static thread_local std::vector<char> received;
	received.clear();
	if (!receiveRequest(received))
	  break;
	Batch response;
	TaskSV::process(_sendFifoName, received, response);
	if (!sendResponse(response))
	  break;
      }
    }
    else {
      while (!_stopFlag) {
	if (!waitRequest())
	  break;
	Batch batch;
	if (!receiveRequest(batch))
	  break;
	Batch response;
	TaskST::process(_sendFifoName, batch, response);
	if (!sendResponse(response))
	  break;
      }
    }
  }
}

} // end of namespace fifo
