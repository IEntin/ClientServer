/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <future>
#include <memory>
#include <string_view>
#include <vector>

using TaskBuilderPtr = std::shared_ptr<class TaskBuilder>;
enum class COMPRESSORS : int;
struct MemoryPool;

class TaskBuilder : public Runnable {

  bool compressSubtask(char* beg, char* end);

  int copyRequestWithId(char* dst, std::string_view line);

  std::vector<char> _task;
  const std::string _sourceName;
  const COMPRESSORS _compressor;
  const bool _diagnostics;
  MemoryPool& _memoryPool;
  bool _done = false;
  std::promise<void> _promise;
  ssize_t _sourcePos;
  ssize_t _requestIndex;
  int _nextIdSz;

 public:

  TaskBuilder(const struct ClientOptions& options,
	      MemoryPool& memoryPool,
	      ssize_t pos,
	      ssize_t  startRequestIndex = 0,
	      int nextIdSz = 4);
  ~TaskBuilder() override;
  void run() noexcept override;
  bool getTask(std::vector<char>& task);
  ssize_t getSourcePos() const { return _sourcePos; }
  ssize_t getRequestIndex() const { return _requestIndex; }
  int getNextIdSz() const { return _nextIdSz; }
  bool createTask();
};
