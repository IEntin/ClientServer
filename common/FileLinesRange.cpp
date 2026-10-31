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

FileLinesRange::FileLinesRange(const std::string& globalSource,
			       char delimiter,
			       int keepDelimiter) :
                               _source(globalSource),
			       _index(0) {
  _lines.clear();
  _linesWithId.clear();
  _linesWithIdReversed.clear();
  _sourceWithIds.clear();
  try {
    const std::size_t numberRows = utility::splitRanges(_source, _lines, delimiter, keepDelimiter);
    for (std::size_t id = 0; id < numberRows; ++id) {
      std::string lineWithId;
      getLineWithId(lineWithId, id);
      _sourceWithIds += lineWithId;
    }
    _linesWithIdReversed = _linesWithId;
    std::reverse(_linesWithIdReversed.begin(), _linesWithIdReversed.end());
    utility::splitRanges(_sourceWithIds, _linesWithId, delimiter, keepDelimiter);
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }
}

bool FileLinesRange::getLine(std::string_view& line) {
  static std::size_t index = 0;
  if (index < _lines.size()) {
    line = _lines[index++];
      return true;
  }
  else {
    index = 0;
    return false;
  }
}

bool FileLinesRange::getLineWithId(std::string& lineWithId, std::size_t index) {
  if (index < _lines.size()) {
    std::string_view line = _lines[index];
    ioutility::updateStringWithId(lineWithId, index, line);
    ++_index;
    return true;
  }
  else
    return false;
}

bool FileLinesRange::getLineWithId(std::vector<std::string_view> linesWithIdReversed,
				   std::string_view lineWithId) {
  if (!linesWithIdReversed.empty()) {
    lineWithId = linesWithIdReversed.back();
    linesWithIdReversed.pop_back();
    return true;
  }
  else
    return false;
}
