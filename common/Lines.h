/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <array>
#include <fstream>
#include <string_view>

// nonallocating analog of getline
// this version saves delimiter in the line
class Lines {
 public:
  Lines(std::string_view fileName, char delimiter = '\n', bool keepDelimiter = false);
  ~Lines() = default;
  // The line can be a string_view or a string depending on
  // the usage. If the line is used up by the app before the
  // next line is created use a string_view, it is
  // backed up by the buffer. Otherwise use a string.
  template <typename S>
  bool getLine(S& line) {
    std::string_view view;
    if (!getLineImpl(view))
      return false;
    line = { view.begin(), view.end() };
    return true;
  }
  bool getLineImpl(std::string_view& line);
  long _index = -1;
  bool _last = false;
 private:
  bool refillBuffer();
  void removeProcessedLines();
  const char _delimiter;
  const bool _keepDelimiter;
  size_t _processed = 0;
  size_t _sizeInUse = 0;
  size_t _totalParsed = 0;
  std::ifstream _stream;
  size_t _fileSize = 0;
  static constexpr unsigned ARRAY_SIZE = 32768;
  std::array<char, ARRAY_SIZE> _buffer;
};
