/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Lines.h"
#include <cassert>
#include <cstring>
#include <filesystem>

Lines::Lines(std::string_view fileName, char delimiter, bool keepDelimiter) :
  _delimiter(delimiter), _keepDelimiter(keepDelimiter) {
  _fileSize = std::filesystem::file_size(fileName);
  _stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  _stream.open(fileName.data(), std::ios::binary);
}

bool Lines::getLineImpl(std::string_view& line) {
  if (_currentPos == 0 ||
      (_totalParsed < _fileSize && _processed >= _bufferRefillThreshold)) {
    removeProcessedLines();
    if (!refillBuffer())
      return false;
  }
  auto itBeg = _buffer.begin() + _processed;
  auto itEnd = std::find(itBeg, _buffer.begin() + _sizeInUse, _delimiter);
  bool endsWithDelimiter = itEnd != _buffer.begin() + _sizeInUse;
  auto dist = std::distance(itBeg, itEnd);
  if (dist == 0)
    return false;
  _totalParsed += dist + (endsWithDelimiter ? 1 : 0);
  if (endsWithDelimiter) {
    ++_index;
    // optionally keep delimiter
    line = { itBeg, itEnd + (_keepDelimiter ? 1 : 0) };
    _processed += dist + 1;
    if (_totalParsed == _fileSize)
      _last = true;
    return true;
  }
  else {
    if (_totalParsed == _fileSize) {
      ++_index;
      line = { itBeg, itEnd };
      _processed += dist;
      _last = true;
    }
    return true;
  }
  return false;
}

// reentrant, thread safe
bool Lines::refillBuffer() {
  if (_buffer.size() == _sizeInUse)
    return false;
  size_t bytesToRead = std::min(_fileSize - _currentPos, _buffer.size() - _sizeInUse);
  if (bytesToRead == 0)
    return false;
  size_t shift = _sizeInUse;
  _sizeInUse += bytesToRead;
  assert(_sizeInUse <= _buffer.size() && "_sizeInUse exceeds ARRAY_SIZE");
  _stream.read(_buffer.data() + shift, bytesToRead);
  _currentPos += bytesToRead;
  assert(_currentPos == static_cast<size_t>(_stream.tellg()));
  return true;
}

void Lines::removeProcessedLines() {
  if (_processed == 0)
    return;
  std::memmove(_buffer.data(), _buffer.data() + _processed, _sizeInUse - _processed);
  _sizeInUse -= _processed;
  _processed = 0;
}