/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FileLines2.h"

#include <filesystem>

FileLines2::FileLines2(std::string_view fileName, char delimiter, bool keepDelimiter) :
  Lines2(delimiter, keepDelimiter) {
  try {
    static thread_local std::string source;
    source.clear();
    utility::readFile(fileName, source);
    utility::splitReversedOrder(source, _lines, _delimiter, _keepDelimiter);
    _maxIndex = _lines.size() - 1;
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }
}

bool FileLines2::getLine(std::string& line) {
  if (_lines.empty()) {
    return false;
  }
  line = std::move(_lines.back());
  _lines.pop_back();
  ++_index;
  _last = _index == _maxIndex;
  return true;
}
