/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <deque>
#include <mutex>
#include <string_view>
#include <vector>
#include <fstream>

enum class STATUS : char;
struct ClientOptions;
struct CryptoKeys;

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
				std::vector<char>& data,
				bool alldone);
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
