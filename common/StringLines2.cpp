/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "StringLines2.h"

StringLines2::StringLines2(std::string_view source, char delimiter, bool keepDelimiter) :
  Lines2(delimiter, keepDelimiter),
  _source(source) {
  try {
    _inputSize = source.size();
    utility::splitReversedOrder(_source, _lines, _delimiter, _keepDelimiter);
    _maxIndex = _lines.size() - 1;
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }
}

bool StringLines2::getLine(std::string& line) {
  if (_index >= _maxIndex)
    return false;
  line = _lines.back();
  _lines.pop_back();
  ++_index;
  _last = _index == _maxIndex;
  return true;
}
