/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include "Subtask.h"
#include <deque>
#include <mutex>
#include <fstream>

enum class STATUS : char;
struct ClientOptions;

class TaskBuilder final : public RunnableT<TaskBuilder> {

  STATUS encryptCompressSubtask(Subtask& subtask, std::string& data, bool alldone);
  int copyRequestWithId(char* dst, std::string_view line);

  const ClientOptions& _options;
  std::ifstream _input;
  std::deque<Subtask> _subtasks;
  ssize_t _requestIndex = 0;
  int _nextIdSz = 4;
  std::mutex _mutex;
  void run() override;
  bool start() override { return true; }
 public:
  TaskBuilder(const ClientOptions& options);
  ~TaskBuilder() override;
  void stop() override {}
  STATUS getSubtask(Subtask& task);
  STATUS createSubtask();
};
