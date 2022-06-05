/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include <atomic>
#include <future>
#include <vector>

using Response = std::vector<std::string>;

class Task;

using TaskPtr = std::shared_ptr<Task>;

using ExtractKey = void (*)(std::string&, std::string_view);

using ProcessRequest = std::string (*)(std::string_view, std::string_view);

struct RequestRow {
  RequestRow(const char* beg, const char* end) : _value(beg, end) {}

  RequestRow(RequestRow&& other) : _value(other._value), _index(other._index) {
    _key.swap(other._key);
  }

  const RequestRow& operator =(RequestRow&& other) {
    _key.swap(other._key);
    _value = other._value;
    _index = other._index;
    return *this;
  }
  std::string _key;
  std::string_view _value;
  int _index{};
};

class Task {
  Task(const Task& other) = delete;
  Task& operator =(const Task& other) = delete;

  size_t size() const { return _rows.size(); }
  bool empty() const { return _rows.empty(); }

  std::vector<RequestRow> _rows;
  std::vector<int> _indices;
  HEADER _header;
  std::atomic<size_t> _pointer = 0;
  std::promise<void> _promise;
  Response& _response;
  static ExtractKey _extractKey;
  static ProcessRequest _processRequest;

 public:
  Task();

  Task(const HEADER& header, std::vector<char>& input, Response& response);

  void sortIndices();

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
