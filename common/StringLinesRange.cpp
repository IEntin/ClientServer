/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "StringLinesRange.h"

#include "Logger.h"
#include "Utility.h"

thread_local std::vector<std::string_view> StringLinesRange::_lines;

StringLinesRange::StringLinesRange(const std::string& source,
				   char delimiter,
				   int keepDelimiter) : _source(source),
							_index(0),
							_last(false ),
							_inputSize(source.size()) {
  _lines.clear();
  try {
    utility::splitRanges(_source, _lines, delimiter, keepDelimiter);
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }
}

bool StringLinesRange::getLine(std::string_view& line) {
  if (_last)
    return false;
  line = _lines[_index++];
  _inputPosition += line.size();
  _last = _inputPosition == _inputSize;
  return true;
}
/*
std::size_t StringLinesRange::getInputPosition() {
  return _inputPosition;
}
*/
