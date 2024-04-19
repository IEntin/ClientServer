/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "StringLines.h"

StringLines::StringLines(std::string_view source) :
  _source(source) {
  _inputSize = source.size();
  refillBuffer();
}

std::size_t StringLines::getInputPosition() {
  return _inputPosition;
}

bool StringLines::refillBuffer() {
  std::size_t bytesToRead = std::min(_inputSize - _inputPosition, _buffer.size() - _sizeInUse);
  auto itBeginSrc = _source.cbegin() + _inputPosition;
  auto itEndSrc = itBeginSrc + bytesToRead;
  auto itBeginDst = _buffer.begin() + _sizeInUse;
  auto itEndDst = std::copy(itBeginSrc, itEndSrc, itBeginDst);
  auto numberCopied[[maybe_unused]] = std::distance(itBeginDst, itEndDst);
  _inputPosition += bytesToRead;
  _sizeInUse += bytesToRead;
  return true;
}
