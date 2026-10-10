/*
 *  Copyright (C) 2021 Ilya Entin
 */
#include <string>
#include <vector>

#pragma once

class FileLinesRange {
 public:
  explicit FileLinesRange(std::string_view fileName, char delimiter = '\n', int keepDelimiter = false);
  ~FileLinesRange()  = default;
  static std::string _source;
  static thread_local std::vector<std::string_view> _lines;
  bool getLine(std::string_view&);
  std::size_t _index = 0;
};
