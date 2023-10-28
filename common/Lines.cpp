/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Lines.h"
#include <cassert>
#include <cstring>
#include <filesystem>

Lines::Lines(std::string_view fileName, char delimiter, bool keepDelimiter) :
  _delimiter(delimiter), _keepDelimiter(keepDelimiter) {
  try {
    _fileSize = std::filesystem::file_size(fileName);
    _stream.open(fileName.data(), std::ios::binary);
    _stream.exceptions(std::ifstream::failbit); // may throw
    refillBuffer();
  }
  catch (const std::ios_base::failure& fail) {
    LogError << fail.what() << '\n';
  }
}

bool Lines::getLineImpl(std::string_view& line) {
  try {
    if (_totalParsed < _fileSize && _processed >= _bufferRefillThreshold) {
      removeProcessedLines();
      refillBuffer();
    }
    auto itBeg = _buffer.begin() + _processed;
    auto itEnd = std::find(itBeg, _buffer.begin() + _sizeInUse, _delimiter);
    bool endsWithDelimiter = itEnd != _buffer.begin() + _sizeInUse;
    auto dist = std::distance(itBeg, itEnd);
    if (dist == 0)
      return false;
    if (endsWithDelimiter) {
      _totalParsed += dist + 1;
      ++_index;
      // optionally keep delimiter
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

// reentrant, thread safe
bool Lines::refillBuffer() {
  try {
    if (_buffer.size() == _sizeInUse)
      return false;
    if (_stream) {
      size_t bytesToRead = std::min(_fileSize - _currentPos, _buffer.size() - _sizeInUse);
      if (bytesToRead == 0)
	return false;
      size_t shift = _sizeInUse;
      _sizeInUse += bytesToRead;
      assert(_sizeInUse <= _buffer.size() && "_sizeInUse exceeds ARRAY_SIZE");
      _stream.read(_buffer.data() + shift, bytesToRead);
      _currentPos += bytesToRead;
    }
    return true;
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
    return false;
  }
}

void Lines::removeProcessedLines() {
  if (_processed == 0)
    return;
  std::memmove(_buffer.data(), _buffer.data() + _processed, _sizeInUse - _processed);
  _sizeInUse -= _processed;
  _processed = 0;
}

// reuse for another (or the same) file
void Lines::reset(std::string_view fileName) {
  _sizeInUse = 0;
  _currentPos = 0;
  _processed = 0;
  _totalParsed = 0;
  _index = -1;
  _last = false;
  try {
    _stream.close();
    _fileSize = std::filesystem::file_size(fileName);
    _stream.open(fileName.data(), std::ios::binary);
    _stream.exceptions(std::ifstream::failbit); // may throw
    refillBuffer();
  }
  catch (const std::ios_base::failure& fail) {
    LogError << fail.what() << '\n';
  }
}
