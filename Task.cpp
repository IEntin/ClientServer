/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Task.h"

#include <algorithm>

#include "EchoPolicy.h"
#include "NoSortInputPolicy.h"
#include "ServerOptions.h"
#include "SortInputPolicy.h"
#include "Transaction.h"
#include "Utility.h"

PreprocessRequest Task::_preprocessRequest =
  ServerOptions::_policyEnum == POLICYENUM::SORTINPUT ? Transaction::createSizeKey : nullptr;
thread_local std::string Task::_buffer;

Task::Task () {
  POLICYENUM policyEnum = ServerOptions::_policyEnum;
  switch (std::to_underlying(policyEnum)) {
  case std::to_underlying(POLICYENUM::NOSORTINPUT) :
    _policy = std::make_unique<NoSortInputPolicy>();
    break;
  case std::to_underlying(POLICYENUM::SORTINPUT) :
    _policy = std::make_unique<SortInputPolicy>();
    break;
  case std::to_underlying(POLICYENUM::ECHOPOLICY) :
    _policy = std::make_unique<EchoPolicy>();
    break;
  default:
    assert(false);
    break;
  }
}

void Task::update(const HEADER& header, std::string_view request) {
  _promise = std::promise<void>();
  _diagnostics = isDiagnosticsEnabled(header);
  _size = utility::splitReuseVector(request, _requests);
  if (ServerOptions::_policyEnum == POLICYENUM::SORTINPUT) {
    _sortedIndices.resize(_size);
    for (unsigned i = 0; i < _size; ++i)
      _sortedIndices[i] = i;
  }
  _response.resize(_size);
}

void Task::sortIndices() {
  std::sort(_sortedIndices.begin(), _sortedIndices.end(), [this] (int idx1, int idx2) {
	      return _requests[idx1]._sizeKey < _requests[idx2]._sizeKey;
	    });
}

bool Task::preprocessNext() {
  unsigned index = _index.fetch_add(1);
  if (index < _size) {
    Request& request = _requests[index];
    request._sizeKey = _preprocessRequest(request._value);
    return true;
  }
  return false;
}

bool Task::processNext() {
  unsigned index = _index.fetch_add(1);
  if (index < _size) {
    if (ServerOptions::_policyEnum == POLICYENUM::SORTINPUT) {
      unsigned orgIndex = _sortedIndices[index];
      Request& request = _requests[orgIndex];
      _response[orgIndex] = _policy->processRequest(request._sizeKey,
						    request._value,
						    _diagnostics,
						    _buffer);
    }
    else {
      SIZETUPLE ZERO_SIZE;
      _response[index] = _policy->processRequest(ZERO_SIZE,
						 _requests[index]._value,
						 _diagnostics,
						 _buffer);
    }
    return true;
  }
  return false;
}

void Task::finish() {
  _promise.set_value();
}
