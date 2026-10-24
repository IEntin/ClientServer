/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "StringLinesRange.h"

std::vector<std::string_view> StringLinesRange::_lines;

bool StringLinesRange::getLine(std::string_view& line) {
  if (_last)
    return false;
  line = _lines[_index++];
  _inputPosition += line.size();
  _last = _inputPosition == _inputSize;
  return true;
}
