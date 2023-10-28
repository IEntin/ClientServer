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
