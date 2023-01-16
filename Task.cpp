/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Task.h"
#include "MemoryPool.h"
#include "Utility.h"

ExtractKey Task::_extractKey = nullptr;
ProcessRequest Task::_processRequest = nullptr;

Task::Task(const HEADER& header, const std::vector<char>& input) : _header(header) {
  utility::split(input, _rows);
  _indices.resize(_rows.size());
  for (int i = 0; i < static_cast<int>(_indices.size()); ++i) {
    _indices[i] = i;
    _rows[i]._index = i;
  }
  _response.resize(_rows.size());
}

void Task::sortIndices() {
  std::sort(_indices.begin(), _indices.end(), [this] (int index1, int index2) {
	      return _rows[index1]._key < _rows[index2]._key;
	    });
}

bool Task::extractKeyNext() {
  if (!_extractKey)
    return false;
  size_t pointer = _pointer.fetch_add(1);
  if (pointer < _rows.size()) {
    RequestRow& row = _rows[pointer];
    std::string_view request = row._value;
    _extractKey(row._key, request);
    return true;
  }
  else
    return false;
}

bool Task::processNext() {
  size_t pointer = _pointer.fetch_add(1);
  if (pointer < _rows.size()) {
    RequestRow& row = _rows[_indices[pointer]];
    std::string_view key = row._key;
    std::string_view request = row._value;
    if (!_processRequest) {
      LogError << "_processRequest is nullptr, Strategy must be set!" << std::endl;
      return false;
    }
    _response[row._index] = _processRequest(key, request);
    return true;
  }
  else
    return false;
}

void Task::finish() {
  try {
    _promise.set_value();
  }
  catch (std::future_error& e) {
    LogError << e.what() << std::endl;
  }
}

void Task::getResponse(Response& response) {
  response.swap(_response);
}
