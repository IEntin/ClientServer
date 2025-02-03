/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Task.h"

#include <algorithm>

#include "Utility.h"

PreprocessRequest Task::_preprocessRequest = nullptr;

ProcessRequest Task::_function;

Response Task::_emptyResponse;

RequestRow::RequestRow(std::string_view::const_iterator beg,
		       std::string_view::const_iterator end) :
  _value(beg, end) {}


Task::Task(Response& response) : _response(response) {}

void Task::setProcessFunction(ProcessRequest function) {
  _function = function;
}

void Task::update(const HEADER& header, std::string_view request) {
  _promise = std::promise<void>();
  _diagnostics = isDiagnosticsEnabled(header);
  utility::splitReuseVector<std::string_view>(request, _rows);
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
    std::size_t typeIndex = _function.index();
    switch (typeIndex) {
    case SORTFUNCTION:
      _response[row._orgIndex] = std::get<SORTFUNCTION>(_function)(row._sizeKey, row._value, _diagnostics);
      break;
    case NOSORTFUNCTION:
      _response[row._orgIndex] = std::get<NOSORTFUNCTION>(_function)(row._value, _diagnostics);
      break;
    case ECHOFUNCTION:
      _response[row._orgIndex] = std::get<ECHOFUNCTION>(_function)(row._value);
      break;
    default:
      return false;
    }
    return true;
  }
  return false;
}

void Task::finish() {
  _promise.set_value();
}
