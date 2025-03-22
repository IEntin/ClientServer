/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Task.h"

#include <algorithm>

#include "ServerOptions.h"
#include "Utility.h"

PreprocessRequest Task::_preprocessRequest;
ProcessRequest Task::_processRequest;
thread_local std::string Task::_buffer;

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
    std::size_t typeIndex = _processRequest.index();
    switch (typeIndex) {
    case SORTFUNCTOR:
      {
	unsigned orgIndex = _sortedIndices[index];
	Request& request = _requests[orgIndex];
	_response[orgIndex] =
	  std::get<SORTFUNCTOR>(_processRequest)(request._sizeKey, request._value, _diagnostics);
      }
      break;
    case NOSORTFUNCTOR:
      _response[index] =
	std::get<NOSORTFUNCTOR>(_processRequest)(_requests[index]._value, _diagnostics);
      break;
    case ECHOFUNCTOR:
      _response[index] =
	std::get<ECHOFUNCTOR>(_processRequest)(_requests[index]._value, _buffer);
      break;
    }
    return true;
  }
  return false;
}

void Task::finish() {
  _promise.set_value();
}
