/*
 *  Copyright (C) 2021 Ilya Entin
 */
#include <string>
#include <vector>
#include "Logger.h"
#include "Utility.h"

#pragma once

class StringLinesRange {
 public:
  template <typename INPUT>
  StringLinesRange(const INPUT& source,
		   char delimiter,
		   int keepDelimiter = 0) : _source(source),
					    _index(0),
					    _last(false ),
					    _inputSize(source.size()) {
    _lines.clear();
    try {
      utility::splitRanges(_source, _lines, delimiter, keepDelimiter);
    }
    catch (const std::exception& e) {
      LogError << e.what() << '\n';
    }
  }

  ~StringLinesRange()  = default;
  std::string_view _source;
  static std::vector<std::string_view> _lines;
  bool getLine(std::string_view&);
  std::size_t _index = 0;
  bool _last = false;
  std::size_t _inputSize = 0;
  std::size_t _inputPosition = 0;
};
