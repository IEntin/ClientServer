/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FileLinesRange.h"

#include "Logger.h"
#include "Utility.h"

std::string FileLinesRange::_source;
thread_local std::vector<std::string_view> FileLinesRange::_lines;

FileLinesRange::FileLinesRange(std::string_view fileName,
			       char delimiter,
			       int keepDelimiter) : _index(0),
						    _last(false) {
  try {
    utility::readFile(fileName, _source);
    utility::splitRanges(_source, _lines, delimiter, keepDelimiter);
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }
}

bool FileLinesRange::getLine(std::string_view& line) {
  if (_index >= _lines.size())
    return false;
  else if (_index == _lines.size() - 1)
    _last = true;
  line = std::move(_lines[_index++]);
  return true;
}
