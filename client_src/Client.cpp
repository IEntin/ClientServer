/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Client.h"
#include "Chronometer.h"
#include "ClientOptions.h"
#include "Compression.h"
#include "TaskBuilder.h"
#include <cassert>
#include <csignal>
#include <cstring>
#include <filesystem>

Client::Client(const ClientOptions& options) : _options(options), _threadPool(1) {
  _memoryPool.setInitialSize(options._bufferSize);
}

Client::~Client() {
  _threadPool.stop();
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool Client::processSubtask(std::vector<char>&& subtask) {
  if (!send(subtask))
    return false;
  if (!receive())
    return false;
  return true;
}

bool Client::processTask(TaskBuilderPtr taskBuilder) {
  std::vector<char> task;
  // Blocks until task construction in another thread is finished
  bool success = taskBuilder->getTask(task);
  if (!success) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ":TaskBuilder failed" << std::endl;
    return false;
  }
  ssize_t sourcePos = taskBuilder->getSourcePos();
  ssize_t requestIndex = taskBuilder->getRequestIndex();
  int nextIdSz = taskBuilder->getNextIdSz();
  if (!processSubtask(std::move(task)))
    return false;
  while (sourcePos < _sourceSize) {
    TaskBuilderPtr builder(std::make_shared<TaskBuilder>(_options, _memoryPool, sourcePos, requestIndex, nextIdSz));
    _threadPool.push(builder);
    bool success = builder->getTask(task);
    if (!success) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ":TaskBuilder failed" << std::endl;
      return false;
    }
    sourcePos = builder->getSourcePos();
    requestIndex = builder->getRequestIndex();
    nextIdSz = builder->getNextIdSz();
    if (!processSubtask(std::move(task)))
      return false;
  }
  return true;
}

bool Client::run() {
  std::error_code ec;
  _sourceSize = std::filesystem::file_size(_options._sourceName, ec);
  if (ec) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ':' << ec.message() << ':' << _options._sourceName << '\n';
    return false;
  }
  int numberTasks = 0;
  TaskBuilderPtr taskBuilder = std::make_shared<TaskBuilder>(_options, _memoryPool, 0);
  _threadPool.push(taskBuilder);
  do {
    Chronometer chronometer(_options._timing, __FILE__, __LINE__, __func__, _options._instrStream);
    TaskBuilderPtr savedBuild(std::move(taskBuilder));
    // start construction of the next task in the background
    if (_options._runLoop) {
      taskBuilder = std::make_shared<TaskBuilder>(_options, _memoryPool, 0);
      _threadPool.push(taskBuilder);
    }
    if (!processTask(savedBuild))
      return false;
    if (_options._maxNumberTasks > 0 && ++numberTasks == _options._maxNumberTasks)
      break;
  } while (_options._runLoop);
  return true;
}

bool Client::printReply(const std::vector<char>& buffer, size_t uncomprSize, size_t comprSize, bool bcompressed) {
  std::string_view received(buffer.data(), comprSize);
  std::ostream* pstream = _options._dataStream;
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
