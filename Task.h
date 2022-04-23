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

using ExtractKey = void (*)(std::string&, std::string_view);

using ProcessRequest = std::string (*)(std::string_view, std::string_view);

class Task {
  enum DATAINDEX { KEY, REQUEST, ORIGINALINDEX };
  using Tuple = std::tuple<std::string, std::string_view, unsigned>;
  using Tuples = std::vector<Tuple>;
  Task(const Task& other) = delete;
  Task& operator =(const Task& other) = delete;

  size_t size() const { return _tuples.size(); }
  bool empty() const { return _tuples.empty(); }

  Tuples _tuples;
  std::vector<unsigned> _indices;
  HEADER _header;
  std::atomic<size_t> _pointer = 0;
  std::promise<void> _promise;
  Batch& _response;
  static ExtractKey _extractKey;
  static ProcessRequest _processRequest;

 public:
  Task(Batch& emptyBatch);

  Task(const HEADER& header, std::vector<char>& input, Batch& response);

  void sortIndices();

  void sortRequests();

  void resetPointer() { _pointer.store(0); }

  bool diagnosticsEnabled() const { return isDiagnosticsEnabled(_header); }

  std::promise<void>& getPromise() { return _promise; }

  bool extractKeyNext();

  bool processNext();

  void finish();

  static void setProcessMethod(ProcessRequest processMethod) {
    _processRequest = processMethod;
  }

  static void setPreprocessMethod(ExtractKey preprocessMethod) {
    _extractKey = preprocessMethod;
  }
};
