/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPoolReference.h"
#include <deque>
#include <mutex>
#include <string_view>
#include <vector>
#include <fstream>

enum class STATUS : char;
struct ClientOptions;
class ThreadPoolBase;

struct Subtask {
  Subtask() = default;
  Subtask(const Subtask&) : _state(STATUS::NONE) {}
  ~Subtask() = default;
  HEADER _header;
  std::vector<char> _body;
  std::atomic<STATUS> _state = STATUS::NONE;
};

class TaskBuilder final : public RunnableT<TaskBuilder> {

  STATUS encryptCompressSubtask(Subtask& subtask,
				const std::vector<char>& data,
				bool alldone);
  int copyRequestWithId(char* dst, std::string_view line);

  const ClientOptions& _options;
  std::ifstream _input;
  std::deque<Subtask> _subtasks;
  ssize_t _requestIndex = 0;
  int _nextIdSz = 4;
  ThreadPoolReference<ThreadPoolBase> _threadPool;
  std::mutex _mutex;
  void run() override;
  bool start() override { return true; }
 public:
  TaskBuilder(const ClientOptions& options, ThreadPoolBase& threadPool);
  ~TaskBuilder() override;
  void stop() override {}
  STATUS getSubtask(Subtask& task);
  STATUS createSubtask();
};
