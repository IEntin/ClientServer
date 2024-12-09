/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <array>
#include <string_view>

// nonallocating analog of getline
// this version optionally saves a delimiter in the line
class Lines {
 public:
  virtual std::size_t getInputPosition() = 0;
  virtual bool refillBuffer() = 0;
  // The line can be a string_view or a string depending on
  // the usage. If the line is used up by the app before the
  // next line is created use a string_view, it is
  // backed up by the buffer. Otherwise use a string.
  template <typename S>
  bool getLine(S& line) {
    auto [success, lineView] = getLineImpl();
    if (!success)
      return false;
    line = lineView;
    return true;
  }
  std::pair<bool, std::string_view> getLineImpl();
  long _index = -1;
  bool _last = false;
 protected:
  Lines(char delimiter = '\n', bool keepDelimiter = false);
  virtual ~Lines() = default;
  bool removeProcessedLines();
  const char _delimiter;
  const bool _keepDelimiter;
  std::size_t _processed = 0;
  std::size_t _sizeInUse = 0;
  std::size_t _inputSize = 0;
  static constexpr unsigned ARRAY_SIZE = 65536;
  std::array<char, ARRAY_SIZE> _buffer;
};
