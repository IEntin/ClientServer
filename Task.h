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

struct Request {

  Request() = default;
  ~Request() = default;

  Request& operator=(std::string_view value) {
    _value = value;
    return *this;
  }

  SIZETUPLE _sizeKey;
  std::string_view _value;
};

class Task : private boost::noncopyable {
  enum Functors {
    SORTFUNCTOR,
    NOSORTFUNCTOR,
    ECHOFUNCTOR
  };
  std::vector<Request> _requests;
  std::size_t _size = 0;
  std::vector<unsigned> _sortedIndices;
  Response _response;
  std::promise<void> _promise;
  std::atomic<unsigned> _index = 0;
  bool _diagnostics;

 public:
  Task() = default;

  ~Task() = default;

  const Response& getResponse() const { return _response; }

  void update(const HEADER& header, std::string_view request);

  void sortIndices();

  void resetIndex() { _index = 0; }

  std::promise<void>& getPromise() { return _promise; }

  bool preprocessNext();

  bool processNext();

  void finish();

  static PreprocessRequest _preprocessRequest;
  static ProcessRequest _processRequest;

  static void setProcessFunctor(ProcessRequest processRequest) {
    _processRequest = processRequest;
  }

  static void setPreprocessFunctor(PreprocessRequest preprocessRequest) {
    _preprocessRequest = preprocessRequest;
  }

};
