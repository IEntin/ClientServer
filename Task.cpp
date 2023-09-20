/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Task.h"
#include "Utility.h"

std::atomic<size_t> Task::_maxSize = 0;
PreprocessRequest Task::_preprocessRequest = nullptr;
ProcessRequest Task::_processRequest = nullptr;
Response Task::_emptyResponse;

Task::Task(Response& response) : _response(response) {}

void Task::set(const HEADER& header, std::string_view input, Response& response) {
  _response = response;
  _diagnostics = isDiagnosticsEnabled(header);
  _rows.reserve(_maxSize);
  utility::split(input, _rows);
  if (_rows.size() > _maxSize)
    _maxSize = _rows.size();
  _indices.resize(_rows.size());
  for (unsigned i = 0; i < _indices.size(); ++i) {
    _indices[i] = i;
    _rows[i]._orgIndex = i;
  }
  _response.get().resize(_rows.size());
}

void Task::sortIndices() {
  std::sort(_indices.begin(), _indices.end(), [this] (int idx1, int idx2) {
	      return _rows[idx1]._key < _rows[idx2]._key;
	    });
}

bool Task::preprocessNext() {
  if (!_preprocessRequest)
    return false;
  unsigned index = _index.fetch_add(1);
  if (index < _rows.size()) {
    RequestRow& row = _rows[index];
    row._key = _preprocessRequest(row._value);
    return true;
  }
  else
    return false;
}

bool Task::processNext() {
  if (!_processRequest) {
    LogError << "_processRequest is nullptr, Strategy must be set!" << '\n';
    return false;
  }
  unsigned index = _index.fetch_add(1);
  if (index < _rows.size()) {
    RequestRow& row = _rows[_indices[index]];
    _response.get()[row._orgIndex] = _processRequest(row._key, row._value, _diagnostics);
    return true;
  }
  else
    return false;
}

void Task::finish() {
  try {
    _promise.set_value();
  }
  catch (const std::future_error& e) {
    LogError << e.what() << '\n';
  }
}
