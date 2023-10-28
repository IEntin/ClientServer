/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Logger.h"
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
  // next line is created a string_view is the choice
  // because it is backed up by the buffer.
  // Otherwise it should be a string.

  template <typename S>
  bool getLine(S& line) {
    if (_last)
      return false;
    try {
      if (_totalParsed < _fileSize && _processed >= _bufferRefillThreshold) {
	removeProcessedLines();
	refillBuffer();
      }
      auto itBeg = _buffer.begin() + _processed;
      auto itEnd = std::find(itBeg, _buffer.begin() + _sizeInUse, _delimiter);
      bool endsWithDelimiter = itEnd != _buffer.begin() + _sizeInUse;
      auto dist = std::distance(itBeg, itEnd);
      if (endsWithDelimiter) {
	_totalParsed += dist + 1;
	++_index;
	// include or don't include delimiter
	line = { itBeg, itEnd + (_keepDelimiter ? 1 : 0) };
	_processed += dist + 1;
	if (_totalParsed == _fileSize)
	  _last = true;
	return true;
      }
      else {
	_totalParsed += dist;
	if (_totalParsed == _fileSize) {
	  ++_index;
	  line = { itBeg, itEnd };
	  _processed += dist;
	  _last = true;
	}
	return true;
      }
    }
    catch (const std::exception& e) {
      LogError << e.what() << '\n';
    }
    return false;
  }

  void reset(std::string_view fileName);
  long _index = -1;
  bool _last = false;
 private:
  bool refillBuffer();
  void removeProcessedLines();
  const char _delimiter;
  const bool _keepDelimiter;
  size_t _currentPos = 0;
  size_t _processed = 0;
  size_t _sizeInUse = 0;
  size_t _totalParsed = 0;
  std::ifstream _stream;
  size_t _fileSize = 0;
  static constexpr unsigned ARRAY_SIZE = 32768;
  std::array<char, ARRAY_SIZE> _buffer;
  static const unsigned _bufferRefillThreshold = ARRAY_SIZE * .9;
};
