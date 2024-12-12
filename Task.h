/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <atomic>
#include <future>
#include <variant>
#include <vector>

#include <boost/core/noncopyable.hpp>

#include "Header.h"

using Response = std::vector<std::string>;

using TaskPtr = std::shared_ptr<class Task>;

using SIZETUPLE = std::tuple<unsigned, unsigned>;

using PreprocessRequest = SIZETUPLE (*)(std::string_view);

using ProcessRequestSort = std::string_view (*)(const SIZETUPLE&, std::string_view, bool diagnostics);

using ProcessRequestNoSort = std::string_view (*)(std::string_view, bool diagnostics);

using ProcessRequestEcho = std::string_view (*)(std::string_view);

using ProcessRequest = std::variant<ProcessRequestSort, ProcessRequestNoSort, ProcessRequestEcho>;

struct RequestRow {

  RequestRow(std::string_view::const_iterator beg, std::string_view::const_iterator end);

  ~RequestRow() {}

  SIZETUPLE _sizeKey;
  std::string_view _value;
  unsigned _orgIndex = 0;
};

class Task : private boost::noncopyable {
  enum Functions {
    SORTFUNCTION,
    NOSORTFUNCTION,
    ECHOFUNCTION
  };
  std::vector<RequestRow> _rows;
  std::vector<unsigned> _indices;
  Response& _response;
  std::promise<void> _promise;
  std::atomic<unsigned> _index = 0;
  bool _diagnostics;
  HEADER _header;
  static ProcessRequest _function;
  static Response _emptyResponse;

 public:
  explicit Task(Response& response = _emptyResponse);

  ~Task();

  void update(std::string_view request);

  void sortIndices();

  void resetIndex() { _index = 0; }

  std::promise<void>& getPromise() { return _promise; }

  bool preprocessNext();

  bool processNext();

  void finish();

  HEADER& header() { return _header; }

  static void setProcessFunction(ProcessRequest function);

  static void setPreprocessFunction(PreprocessRequest function) {
    _preprocessRequest = function;
  }

  static PreprocessRequest _preprocessRequest;
};
