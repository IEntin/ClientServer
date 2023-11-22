/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Task.h"

#include <algorithm>

#include "Utility.h"

PreprocessRequest Task::_preprocessRequest = nullptr;
ProcessRequest Task::_processRequest = nullptr;
Response Task::_emptyResponse;

RequestRow::RequestRow(std::string_view::const_iterator beg,
		       std::string_view::const_iterator end) :
  _value(beg, end) {}


Task::Task(Response& response) : _response(response) {}

Task::~Task() {
  Trace << '\n';
}

void Task::set(const HEADER& header, std::string_view input) {
  _promise = std::promise<void>();
  _diagnostics = isDiagnosticsEnabled(header);
  _rows.clear();
  utility::splitFast(input, _rows);
  _indices.resize(_rows.size());
  for (unsigned i = 0; i < _indices.size(); ++i) {
    _indices[i] = i;
    _rows[i]._orgIndex = i;
  }
  _response.resize(_rows.size());
}

void Task::sortIndices() {
  std::sort(_indices.begin(), _indices.end(), [this] (int idx1, int idx2) {
	      return _rows[idx1]._sizeKey < _rows[idx2]._sizeKey;
	    });
}

bool Task::preprocessNext() {
  unsigned index = _index.fetch_add(1);
  if (index < _rows.size()) {
    RequestRow& row = _rows[index];
    row._sizeKey = _preprocessRequest(row._value);
    return true;
  }
  return false;
}

bool Task::processNext() {
  unsigned index = _index.fetch_add(1);
  if (index < _rows.size()) {
    RequestRow& row = _rows[_indices[index]];
    _response[row._orgIndex] = _processRequest(row._sizeKey, row._value, _diagnostics);
    return true;
  }
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
