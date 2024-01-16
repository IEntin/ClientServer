/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Task.h"

#include <algorithm>

#include "Logger.h"
#include "Utility.h"

PreprocessRequest Task::_preprocessRequest = nullptr;

ProcessRequest Task::_function;

Response Task::_emptyResponse;

RequestRow::RequestRow(std::string_view::const_iterator beg,
		       std::string_view::const_iterator end) :
  _value(beg, end) {}


Task::Task(Response& response) : _response(response) {}

Task::~Task() {
  Trace << '\n';
}

void Task::setProcessFunction(ProcessRequest function) {
  _function = function;
}

void Task::initialize(const HEADER& header, std::string_view input) {
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
    try {
      size_t typeIndex = _function.index();
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
    catch (const std::bad_variant_access& e) {
      LogError << e.what() << '\n';
    }
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
