/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Task.h"

#include <algorithm>

#include "Server.h"
#include "ServerOptions.h"
#include "Transaction.h"

Request::Request(std::string_view input) : _input(input) {
  auto pos = _input.find(']');
  if (pos != std::string_view::npos && _input[0] == '[') {
    _id = { _input.data(), pos + 1 };
    _input.remove_prefix(_id.size());
  }
}

Task::Task (ServerWeakPtr server) : _server(server) {}

std::size_t Task::createRequests(std::string_view input,
				 char delim,
				 int keepDelim) {
  std::size_t index = 0;
  std::size_t start = 0;
  _requests.clear();
  while (start < input.size()) {
    std::size_t next = input.find(delim, start);
    bool endOfInput = next == std::string_view::npos;
    std::string_view line(input.cbegin() + start,
			  endOfInput ? input.cend() : input.cbegin() + next + keepDelim);
    if (!line.empty())
      _requests.emplace_back(line);
    if (endOfInput)
      break;
    else
      ++index;
    start = next + 1;
  }
  return index;
}

void Task::update(const HEADER& header, std::string_view batch) {
  _promise = std::promise<void>();
  _diagnostics = isDiagnosticsEnabled(header);
  _size = createRequests(batch);
  if (ServerOptions::_policyEnum == POLICYENUM::SORTINPUT) {
    _sortedIndices.resize(_size);
    for (std::size_t i = 0; i < _size; ++i)
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
  std::size_t index = _index.fetch_add(1);
  if (index < _size) {
    Request& request = _requests[index];
    request._sizeKey = Transaction::createSizeKey(request._input);
  }
  return _index < _size;
}

bool Task::processNext() {
  std::size_t index = _index.fetch_add(1);
  if (index < _size) {
    if (auto server = _server.lock()) {
      auto& policy = server->getPolicy();
      assert(policy);
      switch (ServerOptions::_policyEnum) {
      case POLICYENUM::SORTINPUT: {
	std::size_t orgIndex = _sortedIndices[index];
	Request& request = _requests[orgIndex];
	_response[orgIndex] = (*policy) (request, _diagnostics);
	break;
      }
      default: {
	Request& request = _requests[index];
	_response[index] = (*policy) (request, _diagnostics);
	break;
      }
      }
    }
  }
  return _index < _size;
}

void Task::finish() {
  _promise.set_value();
}
