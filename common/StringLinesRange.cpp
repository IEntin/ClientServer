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
							_index(0) {
  try {
    utility::splitRanges(_source, _lines, delimiter, keepDelimiter);
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }
}

bool StringLinesRange::getLine(std::string_view& line) {
  if (_index >= _lines.size())
    return false;
  else {
    line = std::move(_lines[_index++]);
    return true;
  }
}
