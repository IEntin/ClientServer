/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FileLinesRange.h"

#include "ClientOptions.h"
#include "IOUtility.h"
#include "Logger.h"
#include "Utility.h"

std::string FileLinesRange::_sourceWithIds;
std::vector<std::string_view> FileLinesRange::_lines;
std::vector<std::string_view> FileLinesRange::_linesWithId;
std::vector<std::string_view> FileLinesRange::_linesWithIdReversed;

FileLinesRange::FileLinesRange(char delimiter,
			       int keepDelimiter) : _index(0),
						    _last(false) {
  _lines.clear();
  _linesWithId.clear();
  _linesWithIdReversed.clear();
  _sourceWithIds.clear();
  try {
    const std::size_t numberRows = utility::splitRanges(_source, _lines, delimiter, keepDelimiter);
    assert(_lines.size() == 10001);
    for (std::size_t id = 0; id < numberRows; ++id) {
      std::string lineWithId;
      getLine(lineWithId, id);
      _sourceWithIds += lineWithId;
    }
    _index = 0;
    _linesWithIdReversed = _linesWithId;
    std::reverse(_linesWithIdReversed.begin(), _linesWithIdReversed.end());
    
    utility::splitRanges(_sourceWithIds, _linesWithId, delimiter, keepDelimiter);
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }
  assert(_lines.size() == _linesWithId.size());
}

bool FileLinesRange::getLine(std::string_view& line) {
  _last = _index >= _lines.size();
  if (_last)
    return false;
  line = _lines[_index];
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

bool FileLinesRange::getLineWithId(std::vector<std::string_view> linesWithIdReversed,
				   std::string_view lineWithId) {
  if (!linesWithIdReversed.empty()) {
    lineWithId = linesWithIdReversed.back();
    linesWithIdReversed.pop_back();
    return true;
  }
  else {
    _last = true;
    return false;
  }
}
