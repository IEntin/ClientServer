/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Task.h"
#include "Utility.h"

Task::Task() : _diagnostics(false) {}

Task::Task(const HEADER& header, std::string_view input) :
  _diagnostics(isDiagnosticsEnabled(header)) {
  utility::split(input, _rows);
  _indices.resize(_rows.size());
  for (int i = 0; i < static_cast<int>(_indices.size()); ++i) {
    _indices[i] = i;
    _rows[i]._orgIndex = i;
  }
  _response.resize(_rows.size());
}

void Task::sortIndices() {
  std::sort(_indices.begin(), _indices.end(), [this] (int idx1, int idx2) {
	      return _rows[idx1]._key < _rows[idx2]._key;
	    });
}

bool Task::preprocessNext() {
  if (!_preprocessRequest)
    return false;
  size_t index = _index.fetch_add(1);
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
  size_t index = _index.fetch_add(1);
  if (index < _rows.size()) {
    RequestRow& row = _rows[_indices[index]];
    _response[row._orgIndex] = _processRequest(row._key, row._value, _diagnostics);
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

void Task::getResponse(Response& response) {
  response.swap(_response);
}
