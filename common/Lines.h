/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/container/static_vector.hpp>
#include <fstream>
#include <string_view>

// nonallocating analog of getline
// this version saves delimiter in the line
class Lines {
 public:
  Lines(std::string_view fileName, char delimiter = '\n');
  ~Lines() = default;
  bool getLine(std::string_view& line);
  void reset(std::string_view fileName);
  long _index = -1;
  bool _last = false;
 private:
  bool refillBuffer();
  void removeProcessedLines();
  const char _delimiter;
  std::string_view _currentLine;
  size_t _currentPos = 0;
  size_t _processed = 0;
  size_t _totalParsed = 0;
  std::ifstream _stream;
  size_t _fileSize = 0;
  static constexpr unsigned STATIC_VECTOR_SIZE = 32768;
  boost::container::static_vector<char, STATIC_VECTOR_SIZE> _buffer;
  static const unsigned _bufferRefillThreshold = STATIC_VECTOR_SIZE * .9;
};
