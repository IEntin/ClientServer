/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FileLinesRange.h"

#include "IOUtility.h"
#include "Logger.h"
#include "Utility.h"

std::string FileLinesRange::_source;
std::size_t FileLinesRange::_inputSize;
std::string FileLinesRange::_sourceWithIds;
std::vector<std::string_view> FileLinesRange::_lines;
std::vector<std::string_view> FileLinesRange::_linesWithId;

FileLinesRange::FileLinesRange(std::string_view fileName,
			       char delimiter,
			       int keepDelimiter) : _index(0),
						    _last(false) {
  _lines.clear();
  try {
    utility::readFile(fileName, _source);
    _inputSize = _source.size();
    std::size_t numberRows = utility::splitRanges(_source, _lines, delimiter, keepDelimiter);
    for (std::size_t id = 0; id < numberRows; ++id) {
      std::string lineWithId;
      getLine(lineWithId, id);
      _sourceWithIds.append(lineWithId);
      _inputPositionWithIds += lineWithId.size();
    }
    _inputSizeWithIds = _sourceWithIds.size();
    
    _index = 0;
    [[maybe_unused]] std::size_t numberRowsWithId = utility::splitRanges(_sourceWithIds, _linesWithId, delimiter, keepDelimiter);
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }
}

bool FileLinesRange::getLine(std::string_view& line) {
  if (_last)
    return false;
  line = _lines[_index];
  _inputPosition += line.size();
  _last = _inputPosition == _inputSize;
  ++_index;
  return true;
}

bool FileLinesRange::getLine(std::string& lineWithId, std::size_t index) {
  if (_last)
    return false;
  std::string_view line = _lines[index];
  _last = line == "\n";
  ioutility::updateStringWithId(lineWithId, index, line);
  ++_index;
  if (_last)
    _index = 0;
  return true;
}
