/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <atomic>
#include <future>
#include <vector>

#include <boost/core/noncopyable.hpp>

#include "Header.h"

using Response = std::vector<std::string>;

using TaskPtr = std::shared_ptr<class Task>;

using SIZETUPLE = std::tuple<unsigned, unsigned>;

using PreprocessRequest = SIZETUPLE (*)(std::string_view);

using ProcessRequest = std::string_view (*)(const SIZETUPLE&, std::string_view, bool diagnostics);

struct RequestRow {
  RequestRow() = default;

  RequestRow(std::string_view::const_iterator beg, std::string_view::const_iterator end);

  ~RequestRow() = default;

  SIZETUPLE _sizeKey;
  std::string_view _value;
  int _orgIndex = 0;
};

class Task : private boost::noncopyable {
  std::vector<RequestRow> _rows;
  std::vector<int> _indices;
  Response& _response;
  std::promise<void> _promise;
  std::atomic<int> _index = 0;
  bool _diagnostics;
  static ProcessRequest _processRequest;
  static Response _emptyResponse;

 public:
  Task(Response& response = _emptyResponse);

  ~Task();

  void set(const HEADER& header, std::string_view input);

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
  static PreprocessRequest _preprocessRequest;
};
