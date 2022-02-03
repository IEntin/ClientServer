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

  size_t size() const { return _storage.size(); }
  bool empty() const { return _storage.empty(); }

  std::tuple<std::string_view, bool, size_t> nextImpl();
  void finishImpl();
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
  static TaskPtr _task;

 public:
  Task();

  Task(const TaskContext& context, std::vector<char>& input, Batch& response);

  static std::tuple<std::string_view, bool, size_t> next() {
    return _task->nextImpl();
  }
  static void push(TaskPtr task);

  static void pop();

  static void updateResponse(size_t index, std::string&& rsp) {
    _task->_response[index].swap(rsp);
  }
  static bool isDiagnosticsEnabled();

  static void finish();

  static void process(const TaskContext& context, std::vector<char>& input, Batch& response);
};
