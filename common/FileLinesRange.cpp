/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FileLinesRange.h"

#include "IOUtility.h"
#include "Logger.h"
#include "Utility.h"

FileLinesRange::FileLinesRange(std::string_view fileName,
			       char delimiter,
			       int keepDelimiter) : _index(0),
						    _last(false),
						    _inputSize(std::filesystem::file_size(fileName)) {
  _lines.clear();
  try {
    utility::readFile(fileName, _source);
    utility::splitRanges(_source, _lines, delimiter, keepDelimiter);
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }
}

bool FileLinesRange::getLine(std::string_view& line) {
  if (_last)
    return false;
  line = _lines[_index++];
  _inputPosition += line.size();
  _last = _inputPosition == _inputSize;
  return true;
}

bool FileLinesRange::getLine(std::string& lineWithId, std::size_t index) {
  if (_last)
    return false;
  std::string_view line = _lines[index++];
  _inputPosition += line.size();
  _last = _inputPosition == _inputSize;
  ioutility::updateStringWithId(lineWithId, index, line);
  return true;
}
