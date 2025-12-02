/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FileLines2.h"

#include <filesystem>

FileLines2::FileLines2(std::string_view fileName, char delimiter, bool keepDelimiter) :
  Lines2(delimiter, keepDelimiter) {
  try {
    _inputSize = std::filesystem::file_size(fileName);
    utility::readFile(fileName, _source);
    utility::splitReversedOrder(_source, _lines, _delimiter, _keepDelimiter);
    _maxIndex = _lines.size() -1;
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }
}

bool FileLines2::getLine(boost::static_string<MAXSUBSTRSIZE>& line) {
  if (_index >= _maxIndex) {
    _source.clear();
    _source.shrink_to_fit();
    return false;
  }
  _substr = _lines.back();
  line = std::move(_substr);
  _lines.pop_back();
  _lines.shrink_to_fit();
  ++_index;
  _last = _index == _maxIndex;
  return true;
}
