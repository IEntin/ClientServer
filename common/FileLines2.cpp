/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FileLines2.h"

#include "Logger.h"
#include "Utility.h"

FileLines2::FileLines2(std::string_view fileName, char delimiter, bool keepDelimiter) :
  Lines2(delimiter, keepDelimiter) {
  try {
    std::string source;
    utility::readFile(fileName, source);
    utility::splitReversedOrder(source, _lines, _delimiter, _keepDelimiter);
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
  _lines.shrink_to_fit();
  ++_index;
  return true;
}
