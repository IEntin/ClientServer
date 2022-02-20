/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
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

  Requests _storage;
  HEADER _header;
  std::atomic<size_t> _pointer = 0;
  std::promise<void> _promise;
  Batch& _response;

 public:
  Task(Batch& emptyBatch);

  Task(const HEADER& header, std::vector<char>& input, Batch& response);

  bool diagnosticsEnabled() const { return isDiagnosticsEnabled(_header); }

  std::promise<void>& getPromise() { return _promise; }

  std::tuple<std::string_view, bool, size_t> next();

  void updateResponse(size_t index, std::string& rsp) {
    _response[index].swap(rsp);
  }

  void finish();
};
