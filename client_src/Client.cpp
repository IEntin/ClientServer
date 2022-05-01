#include "Client.h"
#include "Chronometer.h"
#include "ClientOptions.h"
#include "Compression.h"
#include "TaskBuilder.h"
#include <cassert>
#include <csignal>
#include <cstring>

Client::Client(size_t bufferSize) : _threadPool(1) {
  _memoryPool.setInitialSize(bufferSize);
}

Client::~Client() {
  _threadPool.stop();
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool Client::loop(const ClientOptions& options) {
  unsigned numberTasks = 0;
  TaskBuilderPtr taskBuilder = std::make_shared<TaskBuilder>(options, _memoryPool);
  _threadPool.push(taskBuilder);
  do {
    Chronometer chronometer(options._timing, __FILE__, __LINE__, __func__, options._instrStream);
    // Blocks until task construction in another thread is finished
    bool success = taskBuilder->getTask(_task);
    if (!success) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		 << ":TaskBuilder failed" << std::endl;
      return false;
    }
    // start construction of the next task in the background
    if (options._runLoop) {
      taskBuilder = std::make_shared<TaskBuilder>(options, _memoryPool);
      _threadPool.push(taskBuilder);
    }
    // processes current task
    if (!processTask())
      return false;
    if (options._maxNumberTasks > 0 && ++numberTasks == options._maxNumberTasks)
      break;
  } while (options._runLoop);
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
    std::string_view dstView = Compression::uncompress(received, uncomprSize, _memoryPool);
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
