/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Task.h"
#include "Utility.h"

namespace {

Response emptyResponse;

} // end of anonimous namespace

ExtractKey Task::_extractKey = nullptr;
ProcessRequest Task::_processRequest = nullptr;

Task::Task() : _response(emptyResponse) {}

Task::Task(const HEADER& header, std::vector<char>& input, Response& response) :
  _header(header), _response(response)  {
  static thread_local std::vector<char> rawInput;
  input.swap(rawInput);
  utility::split(std::string_view(rawInput.data(), rawInput.size()), _rows);
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
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << "-exception:" << e.what() << std::endl;
  }
}
