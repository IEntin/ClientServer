#include "Client.h"
#include "Chronometer.h"
#include "ClientOptions.h"
#include "MemoryPool.h"
#include "TaskBuilder.h"
#include <cassert>
#include <csignal>
#include <cstring>
#include <iostream>
#include <sstream>

extern volatile std::sig_atomic_t stopFlag;

Client::Client(size_t bufferSize) : _threadPool(1) {
  MemoryPool::setup(bufferSize);
}

Client::~Client() {
  _threadPool.stop();
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool Client::loop(const ClientOptions& options) {
  unsigned numberTasks = 0;
  TaskBuilderPtr taskBuilder =
    std::make_shared<TaskBuilder>(options._sourceName, options._compressor, options._diagnostics);
  _threadPool.push(taskBuilder);
  do {
    Chronometer chronometer(options._timing, __FILE__, __LINE__, __func__, options._instrStream);
    // blocks until task construction in another thread is finished
    bool success = taskBuilder->getTask(_task);
    if (!success) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		 << ":TaskBuilder failed" << std::endl;
      if (!options._runLoop)
	return false;
    }
    // starts construction of the next task in the background
    if (options._runLoop) {
      taskBuilder =
	std::make_shared<TaskBuilder>(options._sourceName, options._compressor, options._diagnostics);
      _threadPool.push(taskBuilder);
    }
    if (!success)
      continue;
    // processes current task
    if (!processTask())
      return false;
    // limit output file size
    if (++numberTasks == options._maxNumberTasks)
      break;
  } while (options._runLoop && !stopFlag);
  return true;
}

bool Client::printReply(const ClientOptions& options,
			const std::vector<char>& buffer,
			size_t uncomprSize,
			size_t comprSize,
			bool bcompressed) {
  std::string_view received(buffer.data(), comprSize);
  std::ostream* pstream = options._dataStream;
  std::ostream& stream = pstream ? *pstream : std::cout;
  if (bcompressed) {
    static auto& printOnce[[maybe_unused]] = std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
						       << " received compressed" << std::endl;
    std::string_view dstView = Compression::uncompress(received, uncomprSize);
    if (dstView.empty()) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ":failed to uncompress payload" << std::endl;
      return false;
    }
    stream << dstView; 
  }
  else {
    static auto& printOnce[[maybe_unused]] = std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
						       << " received not compressed" << std::endl;
    stream << received;
  }
  return true;
}

std::string Client::readFile(const std::string& name) {
  std::ifstream ifs(name, std::ifstream::in | std::ifstream::binary);
  if (!ifs) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':'
	      << std::strerror(errno) << ' ' << name << std::endl;
    return "";
  }
  std::stringstream buffer;
  buffer << ifs.rdbuf();
  return buffer.str();
}
