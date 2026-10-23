/*
 *  Copyright (C) 2021 Ilya Entin
 */
#include <string>
#include <vector>

#pragma once

class FileLinesRange {
 public:
  explicit FileLinesRange(std::string_view fileName, char delimiter = '\n', int keepDelimiter = 0);
  ~FileLinesRange()  = default;
  static std::string _source;
  static std::string _sourceWithIds;
  static std::vector<std::string_view> _lines;
  static std::vector<std::string_view> _linesWithId;
  bool getLine(std::string_view& line);
  bool getLine(std::string& lineWithId, std::size_t index);
  std::size_t _index = 0;
  bool _last = false;
  static std::size_t _inputSize;
  std::size_t _inputPosition = 0;
  std::size_t _inputSizeWithIds = 0;
  std::size_t _inputPositionWithIds = 0;
};
