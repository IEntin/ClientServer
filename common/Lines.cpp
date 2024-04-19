/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Lines.h"

#include <algorithm>
#include <cstring>

Lines::Lines(char delimiter, bool keepDelimiter) :
  _delimiter(delimiter), _keepDelimiter(keepDelimiter) {}

bool Lines::getLineImpl(std::string_view& line) {
  auto itBeg = _buffer.begin() + _processed;
  auto itEnd = std::find(itBeg, _buffer.begin() + _sizeInUse, _delimiter);
  if (itEnd == _buffer.end() && getInputPosition() < _inputSize) {
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
  if (_processed == _sizeInUse && getInputPosition() == _inputSize)
    _last = true;
  return true;
}

void Lines::removeProcessedLines() {
  if (_processed == 0)
    return;
  std::memmove(_buffer.data(), _buffer.data() + _processed, _sizeInUse - _processed);
  _sizeInUse -= _processed;
  _processed = 0;
}
