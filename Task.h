/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <atomic>
#include <future>
#include <memory>
#include <vector>

#include <boost/core/noncopyable.hpp>

#include "Header.h"
#include "Policy.h"

using Response = std::vector<std::string>;

using PolicyPtr = std::unique_ptr<Policy>;


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
  std::vector<Request> _requests;
  std::size_t _size = 0;
  std::vector<unsigned> _sortedIndices;
  Response _response;
  std::promise<void> _promise;
  std::atomic<unsigned> _index = 0;
  bool _diagnostics;
  static thread_local std::string _buffer;

 public:
  Task ();

  ~Task() = default;

  const Response& getResponse() const { return _response; }

  void update(const HEADER& header, std::string_view request);

  void sortIndices();

  void resetIndex() { _index = 0; }

  std::promise<void>& getPromise() { return _promise; }

  bool preprocessNext();

  bool processNext();

  void finish();

  PolicyPtr _policy;

  static PreprocessRequest _preprocessRequest;

  static void setPreprocessFunctor(PreprocessRequest preprocessRequest) {
    _preprocessRequest = preprocessRequest;
  }

};
