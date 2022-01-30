/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "TaskContext.h"
#include <atomic>
#include <future>
#include <iostream>
#include <queue>

using Batch = std::vector<std::string>;

class Task;

using TaskPtr = std::shared_ptr<Task>;

using Requests = std::vector<std::string_view>;

class Task {
  Task(const Task& other) = delete;
  Task& operator =(const Task& other) = delete;
  static std::mutex _queueMutex;
  static std::condition_variable _queueCondition;
  static std::queue<TaskPtr> _queue;
  Requests _storage;
  static thread_local std::vector<char> _rawInput;
  TaskContext _context;
  std::atomic<size_t> _pointer = 0;
  std::promise<void> _promise;
  Batch& _response;
  static Batch _emptyBatch;
 public:
  Task();

  Task(const TaskContext& context, std::vector<char>& input, Batch& response);

  size_t size() const { return _storage.size(); }

  bool empty() const { return _storage.empty(); }

  bool isDiagnosticsEnabled() const { return _context._diagnostics; }

  std::tuple<std::string_view, bool, size_t> next() {
    size_t pointer = _pointer.fetch_add(1);
    if (pointer < _storage.size()) {
      auto it = std::next(_storage.begin(), pointer);
      return std::make_tuple(std::string_view(it->data(), it->size()),
			     false,
			     std::distance(_storage.begin(), it));
    }
    else
      return std::make_tuple(std::string_view(), true, 0);
  }

  static void push(TaskPtr task);

  static void get(TaskPtr& dest);

  void updateResponse(size_t index, std::string&& rsp) {
    _response[index].swap(rsp);
  }

  void finish();

  static void process(const TaskContext& context, std::vector<char>& input, Batch& response);
};
