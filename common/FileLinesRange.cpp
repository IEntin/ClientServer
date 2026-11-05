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

FileLinesRange::FileLinesRange(const std::string& globalSource,
			       char delimiter,
			       int keepDelimiter) : _source(globalSource),
						    _linesIndex(0),
						    _linesWithIdIndex(0) {
  _lines.clear();
  _linesWithId.clear();
  _sourceWithIds.clear();
  try {
    const std::size_t numberRows = utility::splitRanges(_source, _lines, delimiter, keepDelimiter);
    for (std::size_t id = 0; id < numberRows; ++id) {
      std::string lineWithId;
      createLineWithId(lineWithId, id);
      _sourceWithIds += lineWithId;
    }
    utility::splitRanges(_sourceWithIds, _linesWithId, delimiter, keepDelimiter);
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }
}

bool FileLinesRange::getLine(std::string_view& line) {
  const std::size_t size = _lines.size();
  if (_linesIndex < size) {
    line = _lines[_linesIndex++];
    return true;
  }
  else {
    return false;
  }
}

bool FileLinesRange::getLineWithId(std::string_view& lineWithId) {
  const std::size_t size = _linesWithId.size();
  if (_linesWithIdIndex < size) {
    lineWithId = _linesWithId[_linesWithIdIndex++];
    return true;
  }
  else {
     return false;
  }
}

bool FileLinesRange::createLineWithId(std::string& lineWithId, std::size_t index) {
  const std::size_t size = _lines.size();
  if (index < size) {
    std::string_view line = _lines[index];
    ioutility::updateStringWithId(lineWithId, index, line);
    return true;
  }
  else
    return false;
}

void FileLinesRange::resetIndices() {
  _linesIndex = 0;
  _linesWithIdIndex = 0;
}
