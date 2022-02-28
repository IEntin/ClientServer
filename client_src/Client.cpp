#include "Client.h"
#include "Chronometer.h"
#include "ClientOptions.h"
#include "TaskBuilder.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <sstream>

Client::Client(const ClientOptions& options) : _options(options), _threadPool(1) {}

Client::~Client() {
  _threadPool.stop();
}

bool Client::loop() {
  unsigned numberTasks = 0;
  TaskBuilderPtr taskBuilder =
    std::make_shared<TaskBuilder>(_options._sourceName, _options._compressor, _options._diagnostics);
  _threadPool.push(taskBuilder);
  do {
    Chronometer chronometer(_options._timing, __FILE__, __LINE__, __func__, _options._instrStream);
    // blocks until task construction in another thread is finished
    taskBuilder->getTask(_task);
    if (!taskBuilder->isDone()) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		 << ':' << std::strerror(errno) << std::endl;
      return false;
    }
    // starts construction of the next task in the background
    if (_options._runLoop) {
      taskBuilder =
	std::make_shared<TaskBuilder>(_options._sourceName, _options._compressor, _options._diagnostics);
      _threadPool.push(taskBuilder);
    }
    // processes current task
    if (!processTask())
      return false;
    // limit output file size
    if (++numberTasks == _options._maxNumberTasks)
      break;
  } while (_options._runLoop);
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
