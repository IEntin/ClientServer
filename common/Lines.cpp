/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Lines.h"

#include <algorithm>
#include <cstring>
#include <filesystem>

Lines::Lines(std::string_view fileName, char delimiter, bool keepDelimiter) :
  _delimiter(delimiter), _keepDelimiter(keepDelimiter) {
  _fileSize = std::filesystem::file_size(fileName);
  _stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  _stream.open(fileName.data(), std::ios::binary);
  refillBuffer();
}

bool Lines::getLineImpl(std::string_view& line) {
  auto itBeg = _buffer.begin() + _processed;
  auto itEnd = std::find(itBeg, _buffer.begin() + _sizeInUse, _delimiter);
  if (itEnd == _buffer.end() && static_cast<size_t>(_stream.tellg()) < _fileSize) {
    removeProcessedLines();
    if (!refillBuffer())
      return false;
    itBeg = _buffer.begin() + _processed;
    itEnd = std::find(itBeg, _buffer.begin() + _sizeInUse, _delimiter);
  }
  bool endsWithDelimiter = itEnd != _buffer.end() && *itEnd == _delimiter;
  auto dist = std::distance(itBeg, itEnd);
  if (dist == 0)
    return false;
  ++_index;
  // optionally keep delimiter
  line = { itBeg, itEnd + (endsWithDelimiter && _keepDelimiter ? 1 : 0) };
  _processed += dist + 1;
  if (_processed == _sizeInUse && static_cast<size_t>(_stream.tellg()) == _fileSize)
    _last = true;
  return true;
}

bool Lines::refillBuffer() {
  size_t bytesToRead = std::min(_fileSize - _stream.tellg(), _buffer.size() - _sizeInUse);
  _stream.read(_buffer.data() + _sizeInUse, bytesToRead);
  _sizeInUse += _stream.gcount();
  return true;
}

void Lines::removeProcessedLines() {
  if (_processed == 0)
    return;
  std::memmove(_buffer.data(), _buffer.data() + _processed, _sizeInUse - _processed);
  _sizeInUse -= _processed;
  _processed = 0;
}
