/*
 *  Copyright (C) 2021 Ilya Entin
 */
#include <string>
#include <vector>

#pragma once

class StringLinesRange {
 public:
  explicit StringLinesRange(const std::string& source,
			    char delimiter = '\n',
			    int keepDelimiter = 0);
  ~StringLinesRange()  = default;
  std::string_view _source;
  static thread_local std::vector<std::string_view> _lines;
  bool getLine(std::string_view&);
  std::size_t _index = 0;
};
