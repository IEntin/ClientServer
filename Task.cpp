/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Task.h"

#include <algorithm>

#include "ServerOptions.h"
#include "Utility.h"

PreprocessRequest Task::_preprocessRequest = nullptr;

ProcessRequest Task::_function;

Response Task::_emptyResponse;

Task::Task(Response& response) : _response(response) {}

void Task::setProcessFunction(ProcessRequest function) {
  _function = function;
}

void Task::update(const HEADER& header, std::string_view request) {
  _promise = std::promise<void>();
  _diagnostics = isDiagnosticsEnabled(header);
  _size = utility::splitReuseVector(request, _requests);
  if (ServerOptions::_sortInput) {
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
    std::size_t typeIndex = _function.index();
    switch (typeIndex) {
    case SORTFUNCTION:
      {
	unsigned orgIndex = _sortedIndices[index];
	Request& request = _requests[orgIndex];
	_response[orgIndex] =
	  std::get<SORTFUNCTION>(_function)(request._sizeKey, request._value, _diagnostics);
      }
      break;
    case NOSORTFUNCTION:
      _response[index] =
	std::get<NOSORTFUNCTION>(_function)(_requests[index]._value, _diagnostics);
      break;
    case ECHOFUNCTION:
      _response[index] =
	std::get<ECHOFUNCTION>(_function)(_requests[index]._value);
      break;
    }
    return true;
  }
  return false;
}

void Task::finish() {
  _promise.set_value();
}
