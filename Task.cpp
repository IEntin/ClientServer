/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Task.h"

#include <algorithm>

#include "Server.h"
#include "ServerOptions.h"
#include "Transaction.h"
#include "Utility.h"

PreprocessRequest Task::_preprocessRequest =
  ServerOptions::_policyEnum == POLICYENUM::SORTINPUT ? Transaction::createSizeKey : nullptr;
thread_local std::string Task::_buffer;

Task::Task (ServerWeakPtr server) : _server(server) {}

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
    if (auto server = _server.lock(); server) {
      auto& policy = server->getPolicy();
      assert(policy);
      if (ServerOptions::_policyEnum == POLICYENUM::SORTINPUT) {
	unsigned orgIndex = _sortedIndices[index];
	const Request& request = _requests[orgIndex];
	_response[orgIndex] = (*policy) (request, _diagnostics, _buffer);
      }
      else {
	const Request& request = _requests[index];
	_response[index] = (*policy) (request, _diagnostics, _buffer);
      }
      return true;
    }
  }
  return false;
}

void Task::finish() {
  _promise.set_value();
}
