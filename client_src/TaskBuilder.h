/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include "Subtask.h"
#include <deque>
#include <mutex>
#include <string_view>
#include <vector>
#include <fstream>

enum class STATUS : char;
struct ClientOptions;
struct CryptoKeys;

class TaskBuilder final : public RunnableT<TaskBuilder> {

  STATUS encryptCompressSubtask(Subtask& subtask, std::string& data, bool alldone);
  int copyRequestWithId(char* dst, std::string_view line);

  const ClientOptions& _options;
  CryptoKeys& _cryptoKeys;
  std::ifstream _input;
  std::deque<Subtask> _subtasks;
  ssize_t _requestIndex = 0;
  int _nextIdSz = 4;
  std::mutex _mutex;
  void run() override;
  bool start() override { return true; }
 public:
  TaskBuilder(const ClientOptions& options, CryptoKeys& cryptoKeys);
  ~TaskBuilder() override;
  void stop() override {}
  STATUS getSubtask(Subtask& task);
  STATUS createSubtask();
};
