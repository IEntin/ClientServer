/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include <atomic>
#include <future>
#include <vector>

using Response = std::vector<std::string>;

using TaskPtr = std::shared_ptr<class Task>;

using PreprocessRequest = std::string (*)(std::string_view);

using ProcessRequest = std::string (*)(std::string_view, std::string_view, bool diagnostics);

struct RequestRow {
  RequestRow(const char* beg, const char* end) : _value(beg, end) {}

  RequestRow(RequestRow&& other) : _key(std::move(other._key)),
				   _value(other._value),
				   _orgIndex(other._orgIndex) {}

  RequestRow(const RequestRow& other) = delete;
  RequestRow& operator =(const RequestRow& other) = delete;

  std::string _key;
  std::string_view _value;
  int _orgIndex = 0;
};

class Task {
  Task(const Task& other) = delete;
  Task& operator =(const Task& other) = delete;

  std::vector<RequestRow> _rows;
  std::vector<int> _indices;
  std::atomic<int> _index = 0;
  std::promise<void> _promise;
  bool _diagnostics;
  std::reference_wrapper<Response> _response;
  static PreprocessRequest _preprocessRequest;
  static ProcessRequest _processRequest;
  static Response _emptyResponse;

 public:
  Task(Response& response = _emptyResponse);

  void set(const HEADER& header, std::string_view input, Response& response);

  void sortIndices();

  void resetIndex() { _index = 0; }

  std::promise<void>& getPromise() { return _promise; }

  bool preprocessNext();

  bool processNext();

  void finish();

  static void setProcessMethod(ProcessRequest method) {
    _processRequest = method;
  }

  static void setPreprocessMethod(PreprocessRequest method) {
    _preprocessRequest = method;
  }
};
