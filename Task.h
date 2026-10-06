/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <atomic>
#include <future>
#include <memory>
#include <tuple>

#include <boost/core/noncopyable.hpp>

#include "IOUtility.h"
#include "Header.h"

using SIZETUPLE = std::tuple<unsigned, unsigned>;

using Response = std::vector<std::string>;

using PreprocessRequest = SIZETUPLE (*)(std::string_view);

using ServerWeakPtr = std::weak_ptr<class Server>;

struct Request {

  Request() = default;
  explicit Request(std::string_view input);
  ~Request() = default;

  std::string_view _id;
  SIZETUPLE _sizeKey;
  std::string_view _input;
};

class Task : private boost::noncopyable {
  std::vector<Request> _requests;
  std::size_t _size = 0;
  std::vector<std::size_t> _sortedIndices;
  Response _response;
  std::promise<void> _promise;
  std::atomic<std::size_t> _index = 0;
  bool _diagnostics;
  ServerWeakPtr _server;

 public:
  explicit Task (ServerWeakPtr server = ServerWeakPtr());

  ~Task() = default;

  std::size_t createRequests(std::string_view input,
			     char delim = '\n',
			     int keepDelim = 0);

  const Response& getResponse() const { return _response; }

  void update(const HEADER& header, std::string_view batch);

  void sortIndices();

  void resetIndex() { _index = 0; }

  std::promise<void>& getPromise() { return _promise; }

  bool preprocessNext();

  bool processNext();

  void finish();
};
