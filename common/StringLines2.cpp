/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "StringLines2.h"

#include "Logger.h"
#include "Utility.h"

StringLines2::StringLines2(std::string_view source, char delimiter, bool keepDelimiter) :
  Lines2(delimiter, keepDelimiter) {
  try {
    utility::splitReversedOrder(source, _lines, _delimiter, _keepDelimiter);
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }
}

bool StringLines2::getLine(std::string& line) {
  if (_lines.empty()) {
    return false;
  }
  line = std::move(_lines.back());
  _lines.pop_back();
  ++_index;
  return true;
}
